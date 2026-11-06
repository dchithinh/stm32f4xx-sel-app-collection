#ifndef STM32F407_HAT_DEBUG_UART_H
#define STM32F407_HAT_DEBUG_UART_H

#include "stm32f4xx_hal.h"

/** USART2 on PA2/PA3 — same on Discovery ST-Link VCP and HAT apps */
extern UART_HandleTypeDef huart2;

void debug_uart_init(void);

#endif
