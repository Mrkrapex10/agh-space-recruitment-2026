#include "usart.h"

UART_HandleTypeDef hlpuart1;

/* LPUART1 = ST-LINK virtual COM port on NUCLEO-G474RE (PA2 = TX, PA3 = RX), 115200 8N1 */
void MX_LPUART1_UART_Init(void)
{
    hlpuart1.Instance                    = LPUART1;
    hlpuart1.Init.BaudRate               = 115200;
    hlpuart1.Init.WordLength             = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits               = UART_STOPBITS_1;
    hlpuart1.Init.Parity                 = UART_PARITY_NONE;
    hlpuart1.Init.Mode                   = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (uartHandle->Instance == LPUART1) {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
        PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        __HAL_RCC_LPUART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF12_LPUART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(LPUART1_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);
    }
}
