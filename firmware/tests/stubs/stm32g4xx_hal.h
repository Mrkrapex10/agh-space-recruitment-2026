/* Minimal stand-in for the HAL header, ONLY so that the host unit tests can compile
 * the files that merely include ws2812.h. Not used on the real target. */
#ifndef STUB_STM32G4XX_HAL_H
#define STUB_STM32G4XX_HAL_H
#include <stdint.h>
typedef struct { int unused; } TIM_HandleTypeDef;
#endif
