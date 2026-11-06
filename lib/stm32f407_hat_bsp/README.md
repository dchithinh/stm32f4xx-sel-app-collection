# stm32f407_hat_bsp

Shared **board support** for the [stm32f407_tft_2.4inch_hat](https://github.com/dchithinh/pcb-lab/tree/main/stm32f407_tft_2.4inch_hat): ILI9341 on SPI2, XPT2046 on SPI3 (same bus as microSD on current HAT), and LVGL display/input port.

In this collection it lives at **`lib/stm32f407_hat_bsp`**. For separate app repos, add it as a **git submodule** at the same path (or set `lib_extra_dirs` accordingly).

When using a **submodule**, add the remote in your app repo:

```bash
git submodule add <your-common-bsp-repo-url> lib/stm32f407_hat_bsp
git submodule update --init --recursive
```

Includes use paths like **`lcd/lcd.h`** (not `../lcd/...`). The **BSP package root** `stm32f407_hat_bsp/` must be on the compiler `-I` list, which **`library.json`** sets for the library build and each app’s **`platformio.ini`** mirrors for sources that include BSP headers.

## Build flags (per app)

| Macro | Meaning |
|--------|---------|
| `BSP_LCD_DRAW_BUF_KB` | LCD double buffer size per buffer in KiB (default `24` in `lcd.c` if unset). Music player uses `12`. |
| `BSP_HAS_SD_SPI_BUS` | Set to `1` when the app links a real `sd_spi.c` (shared SPI3 with SD). Pomodoro leaves this unset and owns SPI3 in the touch driver. |
| `LV_CONF_INCLUDE_SIMPLE` / `LV_LVGL_H_INCLUDE_SIMPLE` | Match your project’s LVGL include style (`lvgl.h` vs `lvgl/lvgl.h`). |

Weak stubs in `board/bsp_sd_spi_weak.c` satisfy links for apps without SD; the music player provides strong implementations.

## Layout

- `lcd/` — ILI9341 + pin definitions
- `touch/` — XPT2046
- `port/` — `tft.c`, `touchpad.c`, `lv_drv_conf.h`
- `board/` — HAT identifiers (`hat_board.h`), SD/SPI3 weak stubs, **`debug_uart.c`** / **`debug_uart.h`** (USART2 on PA2/PA3 @ 115200, `printf` via `__io_putchar`)
