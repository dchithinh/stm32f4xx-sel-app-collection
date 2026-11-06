# logger (rxi/log.c + STM32 UART)

This folder vendors **[rxi/log.c](https://github.com/rxi/log.c)** (`log.c` / `log.h`, MIT) and adds **`logger_uart`**: call `logger_init_uart()` after `debug_uart_init()` so logs go over the retargeted UART (`putchar` → `_write` → `__io_putchar`) without using `time()` / `strftime` on the default `stderr` path.

## Usage

1. Add to `platformio.ini`: `lib_deps = ... logger` and `lib_extra_dirs` pointing at the parent of this package (`../lib`).
2. PlatformIO exposes this library’s include dir automatically.
3. In `main`, after UART init:

```c
#include "log.h"
#include "logger_uart.h"

debug_uart_init();
logger_init_uart();
log_info("boot");
```

## Log level

Default threshold is **`LOG_INFO`**. Override with `-D LOGGER_DEFAULT_LEVEL=LOG_DEBUG` (or `LOG_WARN`, etc.) in `build_flags`.

## Macros

Use `log_trace`, `log_debug`, `log_info`, `log_warn`, `log_error`, `log_fatal` instead of `printf` for console diagnostics.
