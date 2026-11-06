#include "main.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"
#include "lvgl.h"
#include "clock_config.h"
#include "debug_uart.h"
#include "tft.h"
#include "touchpad.h"
#include "audio_cs43l22.h"
#include "player.h"
#include "ui_player.h"
#include <stdio.h>

extern DMA_HandleTypeDef lcd_dma_handle;
extern DMA_HandleTypeDef hdma_i2s3_tx;
extern uint32_t _sccmram;
extern uint32_t _eccmram;

void SysTick_Handler(void)
{
    HAL_IncTick();
    lv_tick_inc(1);
}

void DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&lcd_dma_handle);
}

void DMA1_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_i2s3_tx);
}

static void led_on(uint32_t pin)
{
    GPIOD->BSRR = pin;
}

static void early_gpio(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    __DSB();
    /* PD12-15 output */
    GPIOD->MODER = (GPIOD->MODER & ~0xFF000000U) | 0x55000000U;
    GPIOD->OTYPER &= ~0xF000U;
    GPIOD->PUPDR &= ~0xFF000000U;
    led_on(GPIO_PIN_12);
}

void Error_Handler(void)
{
    early_gpio();
    while (1) {
        GPIOD->ODR ^= GPIO_PIN_14;
        for (volatile uint32_t i = 0; i < 200000U; i++) {
        }
    }
}

void HardFault_Handler(void)
{
    early_gpio();
    while (1) {
        GPIOD->ODR ^= GPIO_PIN_15;
        for (volatile uint32_t i = 0; i < 80000U; i++) {
        }
    }
}

static void ccmram_zero(void)
{
    uint32_t *p = &_sccmram;

    if ((uint32_t)&_sccmram < 0x10000000U || (uint32_t)&_eccmram > 0x10010000U) {
        return;
    }
    while (p < &_eccmram) {
        *p++ = 0;
    }
}

static void user_button_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin = GPIO_PIN_0;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
}

int main(void)
{
    uint32_t last_ui = 0;
    uint8_t btn_prev = 0;

    /* BL jumps with IRQs off. Enable FPU and keep the app vector table. */
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
    SCB->VTOR = 0x08010000U;
    __DSB();
    __ISB();

    early_gpio();

    HAL_Init();
    __enable_irq();
    SystemClock_Config();
    ccmram_zero();
    debug_uart_init();
    printf("\r\n=== music_player app ===\r\n");
    printf("SYSCLK=%lu\r\n", (unsigned long)HAL_RCC_GetSysClockFreq());

    user_button_init();

    printf("lvgl/lcd...\r\n");
    lv_init();
    tft_init();
    printf("lcd ok\r\n");

    /* PB9 is LCD CS and CS43L22 SDA — init codec after the display owns PB9. */
    printf("codec...\r\n");
    if (audio_codec_init() != 0) {
        printf("codec init failed (jack may be silent)\r\n");
    } else {
        printf("codec ok\r\n");
    }

    printf("sd/player...\r\n");
    (void)player_init();
    printf("status: %s\r\n", player_get_status_text());

    touchpad_init();
    ui_player_create();
    ui_player_refresh();
    printf("ui ready\r\n");
    led_on(GPIO_PIN_13);

    while (1) {
        uint8_t btn = (uint8_t)HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

        player_poll();
        lv_timer_handler();

        if (btn && !btn_prev) {
            player_toggle();
            ui_player_refresh();
        }
        btn_prev = btn;

        if ((HAL_GetTick() - last_ui) >= 250U) {
            last_ui = HAL_GetTick();
            ui_player_refresh();
            HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
        }
    }
}
