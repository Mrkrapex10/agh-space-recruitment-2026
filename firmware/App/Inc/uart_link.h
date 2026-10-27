/**
 * uart_link - glue between HAL UART and the TLV library:
 *  - RX: interrupt, 1 byte at a time, into a ring buffer
 *  - TX: blocking HAL_UART_Transmit (frames are max 10 bytes)
 */
#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

void uart_link_init(UART_HandleTypeDef *huart);
bool uart_link_read_byte(uint8_t *byte);                              /* false = buffer empty */
int  uart_link_write(void *ctx, const uint8_t *data, size_t len);     /* matches tlv_write_fn */

#endif /* UART_LINK_H */
