#include "main.h"

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* USB-PD dead-battery pull-downs off (frees PB4/PB6 on STM32G4) */
    HAL_PWREx_DisableUCPDDeadBattery();
}
