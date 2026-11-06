/**
 * Shared debug UART: USART2 (PA2 TX, PA3 RX), 115200 8N1.
 * Pins match hat_board.h (Discovery ST-Link virtual COM).
 */
#include "debug_uart.h"
#include "hat_board.h"

#include <stdio.h>

UART_HandleTypeDef huart2;

#ifndef DEBUG_UART_TIMEOUT_MS
#define DEBUG_UART_TIMEOUT_MS  HAL_MAX_DELAY
#endif

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef gpio = {0};

	if (huart->Instance != DBG_UART) {
		return;
	}

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART2_CLK_ENABLE();

	gpio.Pin = DBG_UART_TX_PIN | DBG_UART_RX_PIN;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	gpio.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(DBG_UART_TX_PORT, &gpio);
	/* Polling TX only — no USART2 IRQ (avoids requiring USART2_IRQHandler in every app). */
}

static void dbg_uart_hw_reset(void)
{
	__HAL_RCC_USART2_FORCE_RESET();
	__HAL_RCC_USART2_RELEASE_RESET();
}

void debug_uart_init(void)
{
	dbg_uart_hw_reset();

	huart2.Instance = DBG_UART;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;

	if (HAL_UART_Init(&huart2) != HAL_OK) {
		while (1) {
		}
	}

	setvbuf(stdout, NULL, _IONBF, 0);
}

int __io_putchar(int ch)
{
	uint8_t c = (uint8_t)ch;

	if (HAL_UART_Transmit(&huart2, &c, 1, (uint32_t)DEBUG_UART_TIMEOUT_MS) != HAL_OK) {
		return -1;
	}
	return ch;
}
