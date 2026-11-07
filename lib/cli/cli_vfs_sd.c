#include "cli_vfs_sd.h"
#include "cli_vfs.h"
#include "cli_zmodem.h"

#include "sd_spi.h"
#include "ff.h"

static FATFS s_fs;
static uint8_t s_mounted;

int cli_vfs_sd_mount(void)
{
    sd_spi_init();
    if (f_mount(&s_fs, "", 1) != FR_OK) {
        s_mounted = 0U;
        return -1;
    }
    s_mounted = 1U;
    return 0;
}

bool cli_vfs_sd_is_mounted(void)
{
    return s_mounted != 0U;
}

void cli_vfs_sd_before_io(void)
{
    sd_spi_init();
}

static bool vfs_ready(void)
{
    return cli_vfs_sd_is_mounted();
}

void cli_vfs_sd_register_default(int (*rescan)(int argc, char **argv))
{
    static cli_vfs_hooks_t hooks;
    static uint8_t done;

    if (done) {
        return;
    }
    done = 1U;

    hooks.before_io = cli_vfs_sd_before_io;
    hooks.storage_ready = vfs_ready;
    hooks.rescan = rescan;
    cli_vfs_register(&hooks);

    {
        static cli_zmodem_hooks_t zhooks;

        zhooks.before_io = cli_vfs_sd_before_io;
        zhooks.storage_ready = vfs_ready;
        zhooks.after_receive = NULL;
        zhooks.default_dir = "app";
        cli_zmodem_register(&zhooks);
    }
}
