#include "led_effects.h"

#include <stdbool.h>

#include "app_config.h"
#include "ws2812.h"

static bool     s_blink_active;
static uint32_t s_blink_start_ms;
static uint32_t s_last_update_ms;

/* R: square wave, 50 % duty: full brightness for half of T, then off */
static uint8_t effect_red(uint32_t phase_ms)
{
    return (phase_ms < (LED_PERIOD_MS / 2U)) ? (uint8_t)LED_BRIGHTNESS_MAX : 0U;
}

/* G: sawtooth: linear ramp 0 -> max during T, then instant drop to 0 */
static uint8_t effect_green(uint32_t phase_ms)
{
    return (uint8_t)((phase_ms * LED_BRIGHTNESS_MAX) / LED_PERIOD_MS);
}

/* B: LED_BLINK_COUNT blinks (non-blocking) started by the button */
static uint8_t effect_blue(uint32_t now_ms)
{
    if (!s_blink_active) {
        return 0U;
    }
    uint32_t t = now_ms - s_blink_start_ms;
    uint32_t cycle = LED_BLINK_ON_MS + LED_BLINK_OFF_MS;

    if (t >= (cycle * LED_BLINK_COUNT)) {
        s_blink_active = false;
        return 0U;
    }
    return ((t % cycle) < LED_BLINK_ON_MS) ? (uint8_t)LED_BRIGHTNESS_MAX : 0U;
}

void led_effects_init(void)
{
    s_blink_active = false;
    s_blink_start_ms = 0U;
    s_last_update_ms = 0U;
}

void led_effects_trigger_blink(uint32_t now_ms)
{
    s_blink_active = true;
    s_blink_start_ms = now_ms;
}

void led_effects_compute(uint32_t now_ms, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint32_t phase = now_ms % LED_PERIOD_MS;
    *r = effect_red(phase);
    *g = effect_green(phase);
    *b = effect_blue(now_ms);
}

void led_effects_update(uint32_t now_ms)
{
    if ((uint32_t)(now_ms - s_last_update_ms) < LED_UPDATE_INTERVAL_MS) {
        return;
    }

    uint8_t r, g, b;
    led_effects_compute(now_ms, &r, &g, &b);
    for (uint16_t i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_set_pixel(i, r, g, b);
    }
    if (ws2812_show()) {          /* false = previous transfer still running -> retry next loop */
        s_last_update_ms = now_ms;
    }
}
