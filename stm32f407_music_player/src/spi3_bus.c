#include "spi3_bus.h"
#include "sd_spi.h"
#include "audio_cs43l22.h"
#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef hspi3;

static bool s_i2s;

void spi3_bus_init_spi(void)
{
        s_i2s = false;
        sd_spi_init();
}

void spi3_bus_enter_spi(void)
{
        if (s_i2s) {
                audio_i2s_stop();
        }
        sd_spi_init();
        s_i2s = false;
}

void spi3_bus_enter_i2s(uint32_t audio_hz)
{
        (void)audio_hz;
        if (hspi3.Instance == SPI3) {
                (void)HAL_SPI_DeInit(&hspi3);
        }
        s_i2s = true;
}

bool spi3_bus_is_i2s(void)
{
        return s_i2s;
}
