/**
 * msg_handler - application layer: interprets frames described in msg.h
 * and sends the replies using the TLV library. No HAL dependency.
 */
#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

#include "tlv.h"

#define MSG_MAX_PENDING_PONGS 4U

typedef uint32_t (*msg_now_fn)(void);

typedef struct {
    bool     active;
    uint32_t due_ms;
} msg_pending_pong_t;

typedef struct {
    tlv_t             *tlv;
    msg_now_fn         now_ms;
    msg_pending_pong_t pongs[MSG_MAX_PENDING_PONGS];
} msg_handler_t;

void msg_handler_init(msg_handler_t *h, tlv_t *tlv, msg_now_fn now_ms);

/* Callbacks to plug into tlv_config_t (pass the msg_handler_t as `user`). */
void msg_handler_on_frame(void *user, const tlv_frame_t *frame);
void msg_handler_on_error(void *user, tlv_rx_error_t err, uint8_t type);

/* Call regularly from the main loop: sends PONGs whose delay has elapsed. */
void msg_handler_poll(msg_handler_t *h);

#endif /* MSG_HANDLER_H */
