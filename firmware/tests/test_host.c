/* Host-side unit tests (no MCU needed):  see tests/run_tests.sh */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "led_effects.h"
#include "msg_handler.h"
#include "tlv.h"

/* ---- fake hardware ---- */
static uint32_t g_now;
static uint8_t  g_tx[256];
static size_t   g_tx_len;

static uint32_t fake_tick(void *ctx) { (void)ctx; return g_now; }
static uint32_t fake_now(void) { return g_now; }
static int fake_write(void *ctx, const uint8_t *d, size_t n)
{
    (void)ctx;
    memcpy(&g_tx[g_tx_len], d, n);
    g_tx_len += n;
    return (int)n;
}
/* stubs of the WS2812 driver so led_effects.c links on the host */
void ws2812_set_pixel(uint16_t i, uint8_t r, uint8_t g, uint8_t b) { (void)i; (void)r; (void)g; (void)b; }
bool ws2812_show(void) { return true; }

static tlv_t s_tlv;
static msg_handler_t s_h;

static void setup(void)
{
    tlv_config_t cfg = { .write = fake_write, .get_tick_ms = fake_tick, .on_frame = msg_handler_on_frame,
                         .on_error = msg_handler_on_error, .user = &s_h };
    g_now = 1000; g_tx_len = 0;
    assert(tlv_init(&s_tlv, &cfg) == TLV_OK);
    msg_handler_init(&s_h, &s_tlv, fake_now);
}
static void feed(const uint8_t *d, size_t n) { for (size_t i = 0; i < n; i++) { g_now++; tlv_feed_byte(&s_tlv, d[i]); } }
#define FEED(...) do { const uint8_t _b[] = { __VA_ARGS__ }; feed(_b, sizeof _b); } while (0)
static int expect_tx(int line, const uint8_t *e, size_t n)
{
    int ok = (g_tx_len == n) && (memcmp(g_tx, e, n) == 0);
    if (!ok) {
        printf("FAIL line %d, got %zu bytes:", line, g_tx_len);
        for (size_t i = 0; i < g_tx_len; i++) {
            printf(" %02X", g_tx[i]);
        }
        printf("\n");
    }
    g_tx_len = 0;
    return ok;
}
#define EXPECT_TX(...) do { const uint8_t _e[] = { __VA_ARGS__ }; \
    if (!expect_tx(__LINE__, _e, sizeof _e)) { return 1; } } while (0)

static int test_encode(void)
{
    uint8_t out[TLV_MAX_FRAME_SIZE]; const uint8_t p[] = { 1, 2, 3 };
    assert(tlv_encode(0x10, p, 3, out, sizeof out) == 5);
    assert(out[0] == 0x10 && out[1] == 3 && out[4] == 3);
    assert(tlv_encode(0x10, p, 9, out, sizeof out) == TLV_ERR_TOO_LONG);
    assert(tlv_encode(0x10, p, 3, out, 4) == TLV_ERR_NO_SPACE);
    assert(tlv_encode(0x10, NULL, 0, out, sizeof out) == 2);
    return 0;
}

static int test_ping_pong(void)
{
    setup();
    FEED(0x02, 0x02, 0xE8, 0x03);            /* PING, delay = 1000 ms */
    EXPECT_TX(0x01, 0x02, 0x02, 0x00);       /* ACK(PING, OK) immediately */
    msg_handler_poll(&s_h);
    assert(g_tx_len == 0);                   /* PONG must not be sent yet */
    g_now += 999; msg_handler_poll(&s_h);
    assert(g_tx_len == 0);
    g_now += 5;  msg_handler_poll(&s_h);
    EXPECT_TX(0x03, 0x01, 0x00);             /* PONG after the delay */
    msg_handler_poll(&s_h);
    assert(g_tx_len == 0);                   /* only once */
    return 0;
}

static int test_ping_bad_length(void)
{
    setup();
    FEED(0x02, 0x01, 0xAA);
    EXPECT_TX(0x01, 0x02, 0x02, 0x01);       /* ACK(PING, ERROR) */
    return 0;
}

static int test_math(void)
{
    setup();
    FEED(0x10, 0x05, 0x01, 0x02, 0x00, 0x03, 0x00);        /* 2 + 3 */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x00, 0x05, 0x00);
    FEED(0x10, 0x05, 0x02, 0x02, 0x00, 0x05, 0x00);        /* 2 - 5 = -3 */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x00, 0xFD, 0xFF);
    FEED(0x10, 0x05, 0x04, 0xFC, 0xFF, 0x03, 0x00);        /* -4 * 3 = -12 */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x00, 0xF4, 0xFF);
    FEED(0x10, 0x05, 0x08, 0x64, 0x00, 0xF6, 0xFF);        /* 100 / -10 = -10 */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x00, 0xF6, 0xFF);
    FEED(0x10, 0x05, 0x08, 0x05, 0x00, 0x00, 0x00);        /* 5 / 0 */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x02, 0x00, 0x00);
    FEED(0x10, 0x05, 0x04, 0x2C, 0x01, 0x2C, 0x01);        /* 300 * 300 = 90000 -> overflow */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x01, 0x90, 0x5F);   /* 90000 mod 65536 = 24464 = 0x5F90 */
    FEED(0x10, 0x05, 0x01, 0xFF, 0x7F, 0x01, 0x00);        /* 32767 + 1 -> overflow */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x01, 0x00, 0x80);
    FEED(0x10, 0x05, 0x08, 0x00, 0x80, 0xFF, 0xFF);        /* -32768 / -1 -> overflow */
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x01, 0x00, 0x80);
    return 0;
}

static int test_math_invalid(void)
{
    setup();
    FEED(0x10, 0x05, 0x03, 0x01, 0x00, 0x01, 0x00);        /* two flags */
    EXPECT_TX(0x01, 0x02, 0x10, 0x01);
    FEED(0x10, 0x05, 0x00, 0x01, 0x00, 0x01, 0x00);        /* no flag */
    EXPECT_TX(0x01, 0x02, 0x10, 0x01);
    FEED(0x10, 0x05, 0x11, 0x01, 0x00, 0x01, 0x00);        /* reserved bit set */
    EXPECT_TX(0x01, 0x02, 0x10, 0x01);
    FEED(0x10, 0x03, 0x01, 0x01, 0x00);                    /* wrong length */
    EXPECT_TX(0x01, 0x02, 0x10, 0x01);
    return 0;
}

static int test_unknown_and_replies(void)
{
    setup();
    FEED(0x55, 0x00);                                      /* unknown type, empty payload */
    EXPECT_TX(0x01, 0x02, 0x55, 0x01);
    FEED(0x01, 0x02, 0x02, 0x00);                          /* incoming ACK: ignored (no ACK storm) */
    FEED(0x03, 0x01, 0x00);                                /* incoming PONG: ignored */
    FEED(0x11, 0x03, 0x00, 0x05, 0x00);                    /* incoming MATH_RESP: ignored */
    assert(g_tx_len == 0);
    return 0;
}

static int test_oversize_resync(void)
{
    setup();
    /* type 0x02, length 9 (> 8): 9 junk bytes must be skipped, then a valid MATH frame is parsed */
    FEED(0x02, 0x09, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    EXPECT_TX(0x01, 0x02, 0x02, 0x01);                     /* ACK(type 0x02, ERROR) */
    FEED(0x10, 0x05, 0x01, 0x01, 0x00, 0x01, 0x00);
    EXPECT_TX(0x01, 0x02, 0x10, 0x00,  0x11, 0x03, 0x00, 0x02, 0x00);
    return 0;
}

static int test_timeout_resync(void)
{
    setup();
    FEED(0x10, 0x05, 0x01);                                /* partial MATH frame ... */
    g_now += 500;                                          /* ... line goes quiet */
    FEED(0x02, 0x02, 0x00, 0x00);                          /* valid PING (delay 0) */
    EXPECT_TX(0x01, 0x02, 0x02, 0x00);
    msg_handler_poll(&s_h);
    EXPECT_TX(0x03, 0x01, 0x00);
    return 0;
}

static int test_many_pings_and_wrap(void)
{
    setup();
    g_now = 0xFFFFFF00u;                                   /* tick counter about to wrap */
    FEED(0x02, 0x02, 0x00, 0x02);                          /* delay 512 ms -> due after wrap */
    g_tx_len = 0;
    g_now = 0xFFFFFFF0u; msg_handler_poll(&s_h); assert(g_tx_len == 0);
    g_now = 0x00000200u; msg_handler_poll(&s_h);           /* wrapped */
    EXPECT_TX(0x03, 0x01, 0x00);
    /* 5 pings, only 4 slots */
    setup();
    for (int i = 0; i < 4; i++) { FEED(0x02, 0x02, 0xE8, 0x03); EXPECT_TX(0x01, 0x02, 0x02, 0x00); }
    FEED(0x02, 0x02, 0xE8, 0x03);
    EXPECT_TX(0x01, 0x02, 0x02, 0x01);                     /* no free slot -> ERROR */
    return 0;
}

static int test_led(void)
{
    uint8_t r, g, b;
    led_effects_init();
    led_effects_compute(0, &r, &g, &b);        assert(r == 255 && g == 0 && b == 0);
    led_effects_compute(1499, &r, &g, &b);     assert(r == 255 && g == 127);
    led_effects_compute(1500, &r, &g, &b);     assert(r == 0 && g == 127);
    led_effects_compute(2999, &r, &g, &b);     assert(r == 0 && g == 254);
    led_effects_compute(3000, &r, &g, &b);     assert(r == 255 && g == 0);     /* sawtooth restarts */
    led_effects_compute(4000, &r, &g, &b);     assert(b == 0);                 /* no blink before the button */

    led_effects_trigger_blink(5000);
    int on_edges = 0; uint8_t prev = 0;
    for (uint32_t t = 5000; t < 5000 + 1300; t++) {
        led_effects_compute(t, &r, &g, &b);
        if (b && !prev) on_edges++;
        prev = b;
    }
    assert(on_edges == 3);                     /* exactly 3 blinks */
    led_effects_compute(5000, &r, &g, &b);     /* (blink is over -> inactive) */
    assert(b == 0);
    return 0;
}

int main(void)
{
    int fails = 0;
    fails += test_encode();
    fails += test_ping_pong();
    fails += test_ping_bad_length();
    fails += test_math();
    fails += test_math_invalid();
    fails += test_unknown_and_replies();
    fails += test_oversize_resync();
    fails += test_timeout_resync();
    fails += test_many_pings_and_wrap();
    fails += test_led();
    printf(fails ? "SOME TESTS FAILED\n" : "ALL TESTS PASSED\n");
    return fails ? 1 : 0;
}
