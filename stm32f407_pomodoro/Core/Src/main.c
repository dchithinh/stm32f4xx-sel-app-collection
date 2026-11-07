/* USER CODE BEGIN Header */
/**
    ******************************************************************************
    * @file           : main.c
    * @brief          : Main program body
    ******************************************************************************
    * @attention
    *
    * Copyright (c) 2025 STMicroelectronics.
    * All rights reserved.
    *
    * This software is licensed under terms that can be found in the LICENSE file
    * in the root directory of this software component.
    * If no LICENSE file comes with this software, it is provided AS-IS.
    *
    ******************************************************************************
    */
#include <string.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "lvgl.h"
#include "lv_conf.h"
#include "tft.h"
#include "touchpad.h"
#include "main_screen.h"
#include "debug_utils.h"
#include "clock_config.h"
#include "debug_uart.h"
#include "log.h"
#include "logger_uart.h"
#include "cli.h"
#include "cli_vfs_sd.h"

/**
    * @brief  The application entry point.
    * @retval int
    */
int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Bootloader jumps with PRIMASK=1; cold reset does not. DMA LCD flush needs IRQs. */
    __enable_irq();

    SystemClock_Config();

    debug_uart_init();
    logger_init_uart();
    log_info("Pomodoro app slot 0x08010000, SYSCLK=%lu Hz",
            (unsigned long)HAL_RCC_GetSysClockFreq());

    lv_init();
    log_debug("lv_init OK");
    tft_init();
    log_debug("tft_init OK");
    touchpad_init();
    lv_port_log_init();

    cli_init("pomo> ");
    cli_register_builtin();
    if (cli_vfs_sd_mount() == 0) {
    cli_vfs_sd_register_default(NULL);
    log_info("SD mounted — VFS CLI (ls, cd, mkdir, ...)");
    } else {
    log_warn("SD mount failed — VFS commands unavailable");
    }
    log_info("CLI ready (type help)");

    ui_main_screen(lv_scr_act());

    while (1) {
    if (!cli_is_busy()) {
        cli_poll();
    }
    lv_timer_handler();
    HAL_Delay(5);
    }
}

/**
    * @brief  This function is executed in case of error occurrence.
    * @retval None
    */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef g = { .Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
                            .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL,
                            .Speed = GPIO_SPEED_FREQ_LOW };
    HAL_GPIO_Init(GPIOD, &g);
    __disable_irq();
    while (1)
    {
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13); /* LD3 — fast blink = HAL Error_Handler */
    for (volatile uint32_t d = 0; d < 200000U; d++) {
    }
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
    * @brief  Reports the name of the source file and the source line number
    *         where the assert_param error has occurred.
    * @param  file: pointer to the source file name
    * @param  line: assert_param error line source number
    * @retval None
    */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    log_error("assert_param: file %s line %lu", (const char *)file, (unsigned long)line);
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
