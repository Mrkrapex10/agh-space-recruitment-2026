/**
 * @file tlv.h
 * @brief Small, portable TLV (Type | Length | Value) framing library.
 *
 * Frame on the wire:   byte0 = Type | byte1 = Length N | byte2..N+1 = payload[0..N-1]
 *
 * The library has NO dependency on HAL, UART or any RTOS. Everything hardware
 * specific is injected through function pointers (tlv_config_t), so the same
 * code can be reused on any MCU / transport (UART, SPI, CAN, TCP, a unit test...).
 *
 * Usage:
 *   1. fill tlv_config_t (write function, optional tick function, callbacks)
 *   2. tlv_init()
 *   3. RX: feed every received byte to tlv_feed_byte() / tlv_feed()
 *      -> on_frame() is called for each complete frame
 *   4. TX: tlv_send(type, payload, len)
 */
#ifndef TLV_H
#define TLV_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Max payload size. Can be overridden with -DTLV_MAX_PAYLOAD_SIZE=n
 * (must be identical to the value used by the application, e.g. in msg.h). */
#ifndef TLV_MAX_PAYLOAD_SIZE
#define TLV_MAX_PAYLOAD_SIZE 8
#endif

#if (TLV_MAX_PAYLOAD_SIZE < 0) || (TLV_MAX_PAYLOAD_SIZE > 255)
#error "TLV_MAX_PAYLOAD_SIZE must be in range 0..255 (Length is one byte)"
#endif

#define TLV_HEADER_SIZE           2U
#define TLV_MAX_FRAME_SIZE        (TLV_HEADER_SIZE + TLV_MAX_PAYLOAD_SIZE)
#define TLV_DEFAULT_RX_TIMEOUT_MS 100U

/** Return codes of the TX / encode functions. */
typedef enum {
    TLV_OK           = 0,
    TLV_ERR_ARG      = -1, /* NULL pointer / missing callback */
    TLV_ERR_TOO_LONG = -2, /* payload > TLV_MAX_PAYLOAD_SIZE  */
    TLV_ERR_IO       = -3, /* write callback failed           */
    TLV_ERR_NO_SPACE = -4  /* output buffer too small         */
} tlv_status_t;

/** Errors reported while receiving. */
typedef enum {
    TLV_RX_ERR_LENGTH  = 1, /* declared length > TLV_MAX_PAYLOAD_SIZE (frame is skipped) */
    TLV_RX_ERR_TIMEOUT = 2  /* gap between bytes too long -> partial frame dropped        */
} tlv_rx_error_t;

/** A decoded frame. */
typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t payload[TLV_MAX_PAYLOAD_SIZE];
} tlv_frame_t;

/* ---- interfaces the user of the library provides ---- */

/** Send `len` bytes. Return number of bytes written, or negative on error. */
typedef int (*tlv_write_fn)(void *ctx, const uint8_t *data, size_t len);
/** Monotonic millisecond tick (optional - enables RX timeout). */
typedef uint32_t (*tlv_tick_fn)(void *ctx);
/** Called for every complete, valid frame. `frame` is valid only during the call. */
typedef void (*tlv_frame_cb)(void *user, const tlv_frame_t *frame);
/** Called on RX error (optional). `type` is the type byte of the broken frame. */
typedef void (*tlv_error_cb)(void *user, tlv_rx_error_t err, uint8_t type);

typedef struct {
    tlv_write_fn write;         /* required for TX                  */
    tlv_tick_fn  get_tick_ms;   /* optional (NULL = no RX timeout)  */
    void        *port_ctx;      /* passed to write / get_tick_ms    */
    tlv_frame_cb on_frame;      /* required for RX                  */
    tlv_error_cb on_error;      /* optional                         */
    void        *user;          /* passed to on_frame / on_error    */
    uint32_t     rx_timeout_ms; /* 0 -> TLV_DEFAULT_RX_TIMEOUT_MS   */
} tlv_config_t;

/* ---- instance (treat fields as private) ---- */
typedef enum {
    TLV_STATE_TYPE = 0,
    TLV_STATE_LENGTH,
    TLV_STATE_PAYLOAD,
    TLV_STATE_SKIP
} tlv_rx_state_t;

typedef struct {
    tlv_config_t   cfg;
    tlv_rx_state_t state;
    tlv_frame_t    frame;
    uint8_t        rx_idx;
    uint8_t        skip_left;
    uint32_t       last_rx_ms;
    uint32_t       timeout_ms;
} tlv_t;

tlv_status_t tlv_init(tlv_t *tlv, const tlv_config_t *cfg);
void         tlv_reset(tlv_t *tlv);                              /* drop partial frame */
void         tlv_feed_byte(tlv_t *tlv, uint8_t byte);
void         tlv_feed(tlv_t *tlv, const uint8_t *data, size_t len);

/** Build a frame into `out`. Returns frame size (>0) or a negative tlv_status_t. */
int          tlv_encode(uint8_t type, const void *payload, size_t len,
                        uint8_t *out, size_t out_size);
/** Encode + write in a single write() call. */
tlv_status_t tlv_send(tlv_t *tlv, uint8_t type, const void *payload, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* TLV_H */
