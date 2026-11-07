#ifndef STM32F407_HAT_DEBUG_UART_H
#define STM32F407_HAT_DEBUG_UART_H

#include "stm32f4xx_hal.h"

/** USART2 on PA2/PA3 — same on Discovery ST-Link VCP and HAT apps */
extern UART_HandleTypeDef huart2;

void debug_uart_init(void);

/** Returns 1 and sets *byte if one RX byte is available (non-blocking). */
int debug_uart_try_rx(uint8_t *byte);

/** Blocks up to timeout_ms for one RX byte; returns 1 on success. */
int debug_uart_rx(uint8_t *byte, uint32_t timeout_ms);

void debug_uart_tx_byte(uint8_t byte);

/** Blocking TX of n bytes (one HAL call). */
void debug_uart_write(const uint8_t *data, uint16_t n);

#endif
