#include "ws2812.h"

#include "app_config.h"

#define BITS_PER_LED   24U
#define DMA_BUF_LEN    ((WS2812_LED_COUNT * BITS_PER_LED) + WS2812_RESET_SLOTS)
#define WS2812_BIT_HZ  800000UL /* 1.25 us per bit */

static TIM_HandleTypeDef *s_htim;
static uint32_t           s_channel;
static uint16_t           s_ccr_zero; /* CCR value for a "0" bit (~0.40 us high) */
static uint16_t           s_ccr_one;  /* CCR value for a "1" bit (~0.80 us high) */
static volatile bool      s_busy;

static uint8_t  s_rgb[WS2812_LED_COUNT][3];
static uint16_t s_dma_buf[DMA_BUF_LEN]; /* every element = one CCR value (one bit) */

void ws2812_init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    s_htim = htim;
    s_channel = channel;
    s_busy = false;

    /* ticks per bit, rounded; at 170 MHz: 213 ticks = 1.253 us */
    uint32_t period = (WS2812_TIM_CLK_HZ + (WS2812_BIT_HZ / 2U)) / WS2812_BIT_HZ;
    s_ccr_zero = (uint16_t)((period * 8U) / 25U);   /* 0.32 * 1.25 us = 0.40 us */
    s_ccr_one  = (uint16_t)((period * 16U) / 25U);  /* 0.64 * 1.25 us = 0.80 us */

    __HAL_TIM_SET_AUTORELOAD(htim, period - 1U);
    __HAL_TIM_SET_COMPARE(htim, channel, 0U);
    /* ARR is preloaded: force an update event so the new period is active from the very first bit */
    (void)HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE);

    for (uint16_t i = 0; i < WS2812_LED_COUNT; i++) {
        s_rgb[i][0] = s_rgb[i][1] = s_rgb[i][2] = 0U;
    }
}

void ws2812_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < WS2812_LED_COUNT) {
        s_rgb[index][0] = r;
        s_rgb[index][1] = g;
        s_rgb[index][2] = b;
    }
}

bool ws2812_is_busy(void)
{
    return s_busy;
}

bool ws2812_show(void)
{
    if (s_busy) {
        return false;
    }

    /* Encode: WS2812B expects GRB order, MSB first. */
    uint32_t n = 0U;
    for (uint16_t led = 0; led < WS2812_LED_COUNT; led++) {
        uint32_t grb = ((uint32_t)s_rgb[led][1] << 16) |
                       ((uint32_t)s_rgb[led][0] << 8) |
                       (uint32_t)s_rgb[led][2];
        for (int bit = 23; bit >= 0; bit--) {
            s_dma_buf[n++] = ((grb >> bit) & 1U) ? s_ccr_one : s_ccr_zero;
        }
    }
    /* CCR = 0 -> output stays low = reset/latch time */
    for (uint32_t i = 0; i < WS2812_RESET_SLOTS; i++) {
        s_dma_buf[n++] = 0U;
    }

    s_busy = true;
    if (HAL_TIM_PWM_Start_DMA(s_htim, s_channel, (const uint32_t *)(const void *)s_dma_buf,
                              (uint16_t)DMA_BUF_LEN) != HAL_OK) {
        s_busy = false;
        return false;
    }
    return true;
}

/* Called by HAL (from the DMA interrupt) when the whole buffer has been sent. */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == s_htim) {
        (void)HAL_TIM_PWM_Stop_DMA(htim, s_channel);
        s_busy = false;
    }
}
