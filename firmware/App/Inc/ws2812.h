/**
 * WS2812B driver: TIMx PWM + DMA (no CPU involvement while sending).
 * One timer period = one data bit (800 kHz); the duty cycle encodes 0 / 1.
 */
#ifndef WS2812_H
#define WS2812_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

void ws2812_init(TIM_HandleTypeDef *htim, uint32_t channel);
void ws2812_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
bool ws2812_show(void);      /* false if the previous transfer has not finished yet */
bool ws2812_is_busy(void);

#endif /* WS2812_H */
