#include "app.h"

#include <stdbool.h>

#include "app_config.h"
#include "led_effects.h"
#include "main.h"
#include "msg_handler.h"
#include "tim.h"
#include "tlv.h"
#include "uart_link.h"
#include "usart.h"
#include "ws2812.h"

static tlv_t         s_tlv;
static msg_handler_t s_msg;

/* button debounce state */
static bool     s_btn_raw;
static bool     s_btn_stable;
static uint32_t s_btn_raw_ms;

static uint32_t tlv_tick(void *ctx)
{
    (void)ctx;
    return HAL_GetTick();
}

static uint32_t now_ms(void)
{
    return HAL_GetTick();
}

static bool button_is_pressed(void)
{
    return HAL_GPIO_ReadPin(APP_BTN_PORT, APP_BTN_PIN) == APP_BTN_ACTIVE_STATE;
}

static void button_task(uint32_t now)
{
    bool raw = button_is_pressed();

    if (raw != s_btn_raw) {                 /* level changed: restart the debounce timer */
        s_btn_raw = raw;
        s_btn_raw_ms = now;
    } else if (s_btn_stable != s_btn_raw && (uint32_t)(now - s_btn_raw_ms) >= APP_BTN_DEBOUNCE_MS) {
        s_btn_stable = s_btn_raw;           /* level stable long enough = real edge */
        if (s_btn_stable) {
            led_effects_trigger_blink(now);
        }
    }
}

void app_init(void)
{
    /* --- UART + TLV + message handler --- */
    uart_link_init(&APP_UART_HANDLE);

    const tlv_config_t cfg = {
        .write       = uart_link_write,
        .get_tick_ms = tlv_tick,
        .port_ctx    = NULL,
        .on_frame    = msg_handler_on_frame,
        .on_error    = msg_handler_on_error,
        .user        = &s_msg,
        .rx_timeout_ms = 0U, /* default 100 ms */
    };
    (void)tlv_init(&s_tlv, &cfg);
    msg_handler_init(&s_msg, &s_tlv, now_ms);

    /* --- LED --- */
    ws2812_init(&APP_WS2812_TIM_HANDLE, APP_WS2812_TIM_CHANNEL);
    led_effects_init();

    /* --- button --- */
    s_btn_raw = s_btn_stable = button_is_pressed();
    s_btn_raw_ms = HAL_GetTick();
}

void app_loop(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t  byte;

    while (uart_link_read_byte(&byte)) {
        tlv_feed_byte(&s_tlv, byte);
    }
    msg_handler_poll(&s_msg);
    button_task(now);
    led_effects_update(now);
}
