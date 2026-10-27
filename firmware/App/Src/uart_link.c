#include "uart_link.h"

#define RX_BUF_SIZE     256U /* must be a power of two */
#define TX_TIMEOUT_MS   20U

static UART_HandleTypeDef *s_huart;
static uint8_t             s_rx_byte;
static uint8_t             s_ring[RX_BUF_SIZE];
static volatile uint16_t   s_head; /* written only in the ISR  */
static volatile uint16_t   s_tail; /* written only in main loop */

void uart_link_init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    s_head = 0U;
    s_tail = 0U;
    (void)HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1U);
}

bool uart_link_read_byte(uint8_t *byte)
{
    if (s_tail == s_head) {
        return false;
    }
    *byte = s_ring[s_tail];
    s_tail = (uint16_t)((s_tail + 1U) & (RX_BUF_SIZE - 1U));
    return true;
}

int uart_link_write(void *ctx, const uint8_t *data, size_t len)
{
    (void)ctx;
    if (HAL_UART_Transmit(s_huart, (uint8_t *)(uintptr_t)data, (uint16_t)len, TX_TIMEOUT_MS) == HAL_OK) {
        return (int)len;
    }
    return -1;
}

/* HAL calls this from the UART interrupt after each received byte. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != s_huart) {
        return;
    }
    uint16_t next = (uint16_t)((s_head + 1U) & (RX_BUF_SIZE - 1U));
    if (next != s_tail) {          /* buffer full -> drop the byte */
        s_ring[s_head] = s_rx_byte;
        s_head = next;
    }
    (void)HAL_UART_Receive_IT(huart, &s_rx_byte, 1U); /* re-arm for the next byte */
}

/* Overrun / framing error: clear the flag and keep receiving. */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != s_huart) {
        return;
    }
    __HAL_UART_CLEAR_OREFLAG(huart);
    (void)HAL_UART_Receive_IT(huart, &s_rx_byte, 1U); /* returns HAL_BUSY if already armed - harmless */
}
