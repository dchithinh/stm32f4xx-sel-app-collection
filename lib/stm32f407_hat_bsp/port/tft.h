/**
    * @file disp.h
    * 
    */

#ifndef DISP_H
#define DISP_H

/*********************
    *      INCLUDES
    *********************/
#include <stdint.h>
#if defined(LV_LVGL_H_INCLUDE_SIMPLE) && LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
/* BSP root must be on the include path (see platformio.ini / library.json). */
#include "lcd/lcd.h"
#include "lcd/config.h"

/*********************
    *      DEFINES
    *********************/
#define TFT_HOR_RES 240
#define TFT_VER_RES 320

#define TFT_EXT_FB		0		/*Frame buffer is located into an external SDRAM*/
#define TFT_USE_GPU		0		/*Enable hardware accelerator*/

/**********************
    *      TYPEDEFS
    **********************/

/**********************
    * GLOBAL PROTOTYPES
    **********************/
void tft_init(void);

#if USE_DMA_FLUSH_LCD
void tft_dma_transfer_complete(void);
#endif

/**********************
    *      MACROS
    **********************/

#endif
