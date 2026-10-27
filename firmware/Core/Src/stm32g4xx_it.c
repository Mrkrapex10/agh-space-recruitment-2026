#include "stm32g4xx_it.h"

#include "main.h"

extern DMA_HandleTypeDef hdma_tim1_ch1;
extern UART_HandleTypeDef hlpuart1;

void NMI_Handler(void)        { while (1) { } }
void HardFault_Handler(void)  { while (1) { } }
void MemManage_Handler(void)  { while (1) { } }
void BusFault_Handler(void)   { while (1) { } }
void UsageFault_Handler(void) { while (1) { } }
void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }

void SysTick_Handler(void)
{
    HAL_IncTick(); /* 1 ms tick used by HAL_GetTick() */
}

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_tim1_ch1);
}

void LPUART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&hlpuart1);
}
