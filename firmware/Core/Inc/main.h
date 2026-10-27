#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"

void Error_Handler(void);

/* User button B1 (NUCLEO-G474RE) */
#define B1_Pin        GPIO_PIN_13
#define B1_GPIO_Port  GPIOC

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
