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

/* IRQ ring — ZMODEM + SD write cannot poll HAL_UART_Receive (1-byte HW FIFO overruns). */
#define DBG_RX_SIZE 4096U

static uint8_t s_rx[DBG_RX_SIZE];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;

static void dbg_rx_push(uint8_t b)
{
    uint16_t next = (uint16_t)((s_rx_head + 1U) % DBG_RX_SIZE);

    if (next == s_rx_tail) {
        return;
    }
    s_rx[s_rx_head] = b;
    s_rx_head = next;
}

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

    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
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

    s_rx_head = 0U;
    s_rx_tail = 0U;
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_ERR);

    setvbuf(stdout, NULL, _IONBF, 0);
}

void USART2_IRQHandler(void)
{
    USART_TypeDef *u = USART2;
    uint32_t sr = u->SR;

    if ((sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0U) {
        uint8_t b = (uint8_t)u->DR;

        if ((sr & USART_SR_RXNE) != 0U) {
            dbg_rx_push(b);
        }
    }
}

int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;

    if (HAL_UART_Transmit(&huart2, &c, 1, (uint32_t)DEBUG_UART_TIMEOUT_MS) != HAL_OK) {
        return -1;
    }
    return ch;
}

int debug_uart_try_rx(uint8_t *byte)
{
    uint16_t tail;

    if (byte == NULL) {
        return 0;
    }
    tail = s_rx_tail;
    if (tail == s_rx_head) {
        return 0;
    }
    *byte = s_rx[tail];
    s_rx_tail = (uint16_t)((tail + 1U) % DBG_RX_SIZE);
    return 1;
}

int debug_uart_rx(uint8_t *byte, uint32_t timeout_ms)
{
    uint32_t t0;

    if (byte == NULL) {
        return 0;
    }
    t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < timeout_ms) {
        if (debug_uart_try_rx(byte) != 0) {
            return 1;
        }
    }
    return debug_uart_try_rx(byte);
}

void debug_uart_tx_byte(uint8_t byte)
{
    (void)HAL_UART_Transmit(&huart2, &byte, 1U, 1000U);
}

void debug_uart_write(const uint8_t *data, uint16_t n)
{
    if (data == NULL || n == 0U) {
        return;
    }
    (void)HAL_UART_Transmit(&huart2, (uint8_t *)data, n, 1000U);
}
