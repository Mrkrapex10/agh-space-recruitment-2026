#include "msg_handler.h"

#include <string.h>

#include "msg.h"

/* The wire structs must have exactly the size we expect (packed, no padding). */
_Static_assert(sizeof(packet_t) == 2U + TLV_MAX_PAYLOAD_SIZE, "packet_t layout mismatch");
_Static_assert(sizeof(tlv_frame_t) == sizeof(packet_t), "tlv_frame_t != packet_t");
_Static_assert(sizeof(msg_ack_t) == 2U, "msg_ack_t size");
_Static_assert(sizeof(msg_ping_t) == 2U, "msg_ping_t size");
_Static_assert(sizeof(msg_pong_t) == 1U, "msg_pong_t size");
_Static_assert(sizeof(msg_math_t) == 5U, "msg_math_t size");
_Static_assert(sizeof(msg_math_resp_t) == 3U, "msg_math_resp_t size");

/* ------------------------------------------------------------------ */

static void send_ack(msg_handler_t *h, uint8_t acked_id, ack_status_t status)
{
    msg_ack_t ack = { .ack_msg_id = acked_id, .ack_status = (uint8_t)status };
    (void)tlv_send(h->tlv, MSG_ACK, &ack, sizeof(ack));
}

static bool schedule_pong(msg_handler_t *h, uint16_t delay_ms)
{
    for (unsigned i = 0; i < MSG_MAX_PENDING_PONGS; i++) {
        if (!h->pongs[i].active) {
            h->pongs[i].active = true;
            h->pongs[i].due_ms = h->now_ms() + delay_ms;
            return true;
        }
    }
    return false; /* no free slot */
}

static void handle_ping(msg_handler_t *h, const tlv_frame_t *f)
{
    msg_ping_t ping;

    if (f->length != sizeof(ping)) {
        send_ack(h, f->type, MSG_STATUS_ERROR);
        return;
    }
    memcpy(&ping, f->payload, sizeof(ping)); /* memcpy: no unaligned access to packed data */

    if (!schedule_pong(h, ping.pong_delay_ms)) {
        send_ack(h, f->type, MSG_STATUS_ERROR);
        return;
    }
    send_ack(h, f->type, MSG_STATUS_OK);
}

static void handle_math(msg_handler_t *h, const tlv_frame_t *f)
{
    msg_math_t     req;
    msg_math_resp_t resp;

    if (f->length != sizeof(req)) {
        send_ack(h, f->type, MSG_STATUS_ERROR);
        return;
    }
    memcpy(&req, f->payload, sizeof(req));

    /* exactly ONE operation flag must be set and the reserved bits must be 0 */
    unsigned ops = (unsigned)req.operation_bits.add + (unsigned)req.operation_bits.sub +
                   (unsigned)req.operation_bits.mul + (unsigned)req.operation_bits.div;
    if (ops != 1U || req.operation_bits.reserved != 0U) {
        send_ack(h, f->type, MSG_STATUS_ERROR);
        return;
    }

    int32_t a = req.operand_a;
    int32_t b = req.operand_b;
    int32_t r = 0;
    memset(&resp, 0, sizeof(resp));

    if (req.operation_bits.add) {
        r = a + b;
    } else if (req.operation_bits.sub) {
        r = a - b;
    } else if (req.operation_bits.mul) {
        r = a * b;
    } else { /* div */
        if (b == 0) {
            resp.flags_bits.divide_by_zero = 1U;
            r = 0;
        } else {
            r = a / b; /* INT16_MIN / -1 = 32768 fits in int32 -> caught as overflow below */
        }
    }

    if (r > INT16_MAX || r < INT16_MIN) {
        resp.flags_bits.overflow = 1U;
    }
    resp.result = (int16_t)r; /* on overflow: two's complement wrap-around */

    send_ack(h, f->type, MSG_STATUS_OK);
    (void)tlv_send(h->tlv, MSG_MATH_RESP, &resp, sizeof(resp));
}

/* ------------------------------------------------------------------ */

void msg_handler_init(msg_handler_t *h, tlv_t *tlv, msg_now_fn now_ms)
{
    memset(h, 0, sizeof(*h));
    h->tlv = tlv;
    h->now_ms = now_ms;
}

void msg_handler_on_frame(void *user, const tlv_frame_t *frame)
{
    msg_handler_t *h = (msg_handler_t *)user;
    if (h == NULL || frame == NULL) {
        return;
    }

    switch (frame->type) {
    case MSG_PING:
        handle_ping(h, frame);
        break;
    case MSG_MATH:
        handle_math(h, frame);
        break;
    case MSG_ACK:
    case MSG_PONG:
    case MSG_MATH_RESP:
        /* These are replies, not commands. Never ACK a reply - two devices
         * ACKing each other's ACKs would ping-pong forever. */
        break;
    default:
        send_ack(h, frame->type, MSG_STATUS_ERROR); /* unknown message id */
        break;
    }
}

void msg_handler_on_error(void *user, tlv_rx_error_t err, uint8_t type)
{
    msg_handler_t *h = (msg_handler_t *)user;
    if (h == NULL) {
        return;
    }
    if (err == TLV_RX_ERR_LENGTH) {
        send_ack(h, type, MSG_STATUS_ERROR); /* frame too long: tell the sender */
    }
    /* TLV_RX_ERR_TIMEOUT: partial frame dropped silently */
}

void msg_handler_poll(msg_handler_t *h)
{
    uint32_t now = h->now_ms();

    for (unsigned i = 0; i < MSG_MAX_PENDING_PONGS; i++) {
        /* signed difference = correct also when the 32-bit ms counter wraps */
        if (h->pongs[i].active && (int32_t)(now - h->pongs[i].due_ms) >= 0) {
            msg_pong_t pong = { .reserved = 0U };
            h->pongs[i].active = false;
            (void)tlv_send(h->tlv, MSG_PONG, &pong, sizeof(pong));
        }
    }
}
