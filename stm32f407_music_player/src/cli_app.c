#include "cli_app.h"
#include "cli.h"
#include "cli_vfs.h"
#include "cli_zmodem.h"
#include "log.h"
#include "player.h"
#include "spi3_bus.h"

#include "stm32f4xx_hal.h"
#include <stdio.h>

static void vfs_before_io(void)
{
    player_stop();
    {
        uint32_t t0 = HAL_GetTick();
        while (player_spi_busy() && (HAL_GetTick() - t0) < 2000U) {
        }
    }
    spi3_bus_enter_spi();
}

static bool vfs_storage_ready(void)
{
    return player_sd_mounted();
}

static int vfs_rescan(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    player_rescan_tracks();
    printf("tracks: %u — %s\r\n", (unsigned)player_get_track_count(), player_get_status_text());
    return 0;
}

static void after_zmodem(void)
{
    player_rescan_tracks();
}

static int cmd_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    player_stop();
    printf("player stopped\r\n");
    return 0;
}

static const cli_command_t s_app_cmds[] = {
    { "stop", "stop — stop audio playback", cmd_stop },
};

void cli_app_register(void)
{
    static const cli_vfs_hooks_t hooks = {
        .before_io = vfs_before_io,
        .storage_ready = vfs_storage_ready,
        .rescan = vfs_rescan,
    };

    static const cli_zmodem_hooks_t zhooks = {
        .before_io = vfs_before_io,
        .storage_ready = vfs_storage_ready,
        .after_receive = after_zmodem,
        .default_dir = "music",
    };

    cli_vfs_register(&hooks);
    cli_zmodem_register(&zhooks);
    cli_register(s_app_cmds, sizeof(s_app_cmds) / sizeof(s_app_cmds[0]));
}
