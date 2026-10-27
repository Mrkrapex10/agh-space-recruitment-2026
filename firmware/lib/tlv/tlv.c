#include "tlv.h"

#include <string.h>

static void report_error(tlv_t *t, tlv_rx_error_t err, uint8_t type)
{
    if (t->cfg.on_error != NULL) {
        t->cfg.on_error(t->cfg.user, err, type);
    }
}

static void deliver_frame(tlv_t *t)
{
    t->state = TLV_STATE_TYPE; /* ready for the next frame before user code runs */
    if (t->cfg.on_frame != NULL) {
        t->cfg.on_frame(t->cfg.user, &t->frame);
    }
}

tlv_status_t tlv_init(tlv_t *tlv, const tlv_config_t *cfg)
{
    if (tlv == NULL || cfg == NULL) {
        return TLV_ERR_ARG;
    }
    memset(tlv, 0, sizeof(*tlv));
    tlv->cfg = *cfg;
    tlv->timeout_ms = (cfg->rx_timeout_ms != 0U) ? cfg->rx_timeout_ms : TLV_DEFAULT_RX_TIMEOUT_MS;
    tlv->state = TLV_STATE_TYPE;
    return TLV_OK;
}

void tlv_reset(tlv_t *tlv)
{
    if (tlv == NULL) {
        return;
    }
    tlv->state = TLV_STATE_TYPE;
    tlv->rx_idx = 0U;
    tlv->skip_left = 0U;
}

void tlv_feed_byte(tlv_t *t, uint8_t byte)
{
    if (t == NULL) {
        return;
    }

    /* Resynchronisation: the protocol has no start marker or CRC, so if the
     * line goes quiet in the middle of a frame we throw the partial frame away. */
    if (t->cfg.get_tick_ms != NULL) {
        uint32_t now = t->cfg.get_tick_ms(t->cfg.port_ctx);
        if (t->state != TLV_STATE_TYPE && (uint32_t)(now - t->last_rx_ms) > t->timeout_ms) {
            report_error(t, TLV_RX_ERR_TIMEOUT, t->frame.type);
            tlv_reset(t);
        }
        t->last_rx_ms = now;
    }

    switch (t->state) {
    case TLV_STATE_TYPE:
        t->frame.type = byte;
        t->state = TLV_STATE_LENGTH;
        break;

    case TLV_STATE_LENGTH:
        if (byte > TLV_MAX_PAYLOAD_SIZE) {
            /* Too long for us: report it and swallow `byte` payload bytes so the
             * stream stays aligned with the frame boundaries. */
            report_error(t, TLV_RX_ERR_LENGTH, t->frame.type);
            t->skip_left = byte;
            t->state = TLV_STATE_SKIP;
        } else {
            t->frame.length = byte;
            t->rx_idx = 0U;
            if (byte == 0U) {
                deliver_frame(t);
            } else {
                t->state = TLV_STATE_PAYLOAD;
            }
        }
        break;

    case TLV_STATE_PAYLOAD:
        t->frame.payload[t->rx_idx++] = byte;
        if (t->rx_idx >= t->frame.length) {
            deliver_frame(t);
        }
        break;

    case TLV_STATE_SKIP:
        if (--t->skip_left == 0U) {
            t->state = TLV_STATE_TYPE;
        }
        break;

    default:
        tlv_reset(t);
        break;
    }
}

void tlv_feed(tlv_t *tlv, const uint8_t *data, size_t len)
{
    if (tlv == NULL || data == NULL) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        tlv_feed_byte(tlv, data[i]);
    }
}

int tlv_encode(uint8_t type, const void *payload, size_t len, uint8_t *out, size_t out_size)
{
    if (out == NULL || (len > 0U && payload == NULL)) {
        return TLV_ERR_ARG;
    }
    if (len > TLV_MAX_PAYLOAD_SIZE) {
        return TLV_ERR_TOO_LONG;
    }
    if (out_size < (TLV_HEADER_SIZE + len)) {
        return TLV_ERR_NO_SPACE;
    }
    out[0] = type;
    out[1] = (uint8_t)len;
    if (len > 0U) {
        memcpy(&out[TLV_HEADER_SIZE], payload, len);
    }
    return (int)(TLV_HEADER_SIZE + len);
}

tlv_status_t tlv_send(tlv_t *tlv, uint8_t type, const void *payload, size_t len)
{
    uint8_t buf[TLV_MAX_FRAME_SIZE];

    if (tlv == NULL || tlv->cfg.write == NULL) {
        return TLV_ERR_ARG;
    }
    int n = tlv_encode(type, payload, len, buf, sizeof(buf));
    if (n < 0) {
        return (tlv_status_t)n;
    }
    int written = tlv->cfg.write(tlv->cfg.port_ctx, buf, (size_t)n);
    return (written == n) ? TLV_OK : TLV_ERR_IO;
}
