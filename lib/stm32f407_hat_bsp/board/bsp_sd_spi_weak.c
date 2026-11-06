/**
 * Weak stubs for sd_spi when an app does not use the microSD driver (SPI3 owned in touch).
 * When BSP_HAS_SD_SPI_BUS=1, the real sd_spi.c in the app overrides these.
 */
#include <stdint.h>

#if !defined(BSP_HAS_SD_SPI_BUS) || BSP_HAS_SD_SPI_BUS == 0

#include "sd_spi.h"

__attribute__((weak)) void sd_spi_init(void) {}
__attribute__((weak)) void sd_spi_cs_release_all(void) {}
__attribute__((weak)) void sd_spi_cs_assert(uint16_t cs_pin) { (void)cs_pin; }
__attribute__((weak)) void sd_spi_cs_select(void) {}
__attribute__((weak)) void sd_spi_cs_deselect(void) {}
__attribute__((weak)) uint8_t sd_spi_xchg(uint8_t data) { (void)data; return 0xFF; }
__attribute__((weak)) void sd_spi_rx(uint8_t *buf, uint32_t len) { (void)buf; (void)len; }
__attribute__((weak)) void sd_spi_tx(const uint8_t *buf, uint32_t len) { (void)buf; (void)len; }
__attribute__((weak)) void sd_spi_set_slow(void) {}
__attribute__((weak)) void sd_spi_set_fast(void) {}
__attribute__((weak)) void sd_spi_set_touch(void) {}
__attribute__((weak)) void sd_spi_bus_idle_clocks(uint32_t n) { (void)n; }

#endif
