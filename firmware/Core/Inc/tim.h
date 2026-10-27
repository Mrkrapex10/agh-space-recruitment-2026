#ifndef TIM_H
#define TIM_H
#include "main.h"

extern TIM_HandleTypeDef htim1;

void MX_TIM1_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#endif
