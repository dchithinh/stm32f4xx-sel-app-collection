#ifndef CLI_VFS_SD_H
#define CLI_VFS_SD_H

#include <stdbool.h>

/** Init SPI3 SD (hat) and mount FatFs volume `""`. Returns 0 on success. */
int cli_vfs_sd_mount(void);

bool cli_vfs_sd_is_mounted(void);

/** Re-select SPI mode before SD access (safe when no I2S on SPI3). */
void cli_vfs_sd_before_io(void);

/**
    * `cli_vfs_register()` + `rz` (default dir `app`) with default SD hooks.
    * Optional `rescan` hook for VFS `rescan` command.
    */
void cli_vfs_sd_register_default(int (*rescan)(int argc, char **argv));

#endif
