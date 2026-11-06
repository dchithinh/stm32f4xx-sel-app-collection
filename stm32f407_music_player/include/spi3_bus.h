#ifndef SPI3_BUS_H
#define SPI3_BUS_H

#include <stdbool.h>
#include <stdint.h>

void spi3_bus_init_spi(void);
void spi3_bus_enter_spi(void);
void spi3_bus_enter_i2s(uint32_t audio_hz);
bool spi3_bus_is_i2s(void);

#endif
