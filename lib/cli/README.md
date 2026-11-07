# cli

Interactive serial shell on **USART2** (115200 8N1): core commands plus **microSD / FatFs** VFS.

## Core

```c
#include "cli.h"

cli_init("pomo> ");
cli_register_builtin();   /* help, echo, uptime */
cli_register(my_cmds, n); /* optional app commands */
```

Poll `cli_poll()` from the main loop (`cli_set_busy()` during blocking work).

**Tera Term (required for Backspace):**
- Local echo **OFF**
- Setup → Terminal → New-line: **Receive = CR**, **Transmit = CR** (not LF / AUTO)

Letters are echoed by the MCU. Backspace redraws the line with `\r` (ST-Link drops Pico’s `\b \b`).

## SD / VFS (Hat + card)

```c
#include "cli_vfs_sd.h"

if (cli_vfs_sd_mount() == 0) {
  cli_vfs_sd_register_default(NULL);  /* VFS + rz (default dir app/) */
}
```

Requires `lib_deps = cli, sdcard, fatfs, stm32f407_hat_bsp` and `-D BSP_HAS_SD_SPI_BUS=1`.

### ZMODEM

On the device: `rz` or `rz app/` (creates the folder if needed). On the host: **start `sz` / Tera Term ZMODEM send only after** “Waiting for sender…”.

**Speed:** 115200 8N1 is about **11.5 KB/s** on the wire; Tera Term showing ~10 KB/s is expected. A stall after a few hundred bytes was UART overrun (1-byte poll vs slow SD write) — RX is now a 4 KiB IRQ ring.

Music player: custom `cli_zmodem_register()` with default `music` and an `after_receive` hook (see `cli_app.c`).

Custom bus sharing (music player): `#include "cli_vfs.h"` and `cli_vfs_register(&hooks)` instead of `cli_vfs_sd_register_default`.

## Headers

| Header | Purpose |
|--------|---------|
| `cli.h` | Line editor, register commands |
| `cli_vfs.h` | VFS commands + hooks |
| `cli_vfs_sd.h` | Mount SD + default VFS + `rz` |
| `cli_zmodem.h` | `rz` only (custom hooks / default dir) |
