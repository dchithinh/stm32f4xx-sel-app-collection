#include "cli_zmodem.h"
#include "cli.h"

#include "zmodem_fatfs.h"

#include "ff.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

#define RZ_DEFAULT_DIR "app"

static cli_zmodem_hooks_t s_hooks;
static uint8_t s_hooks_set;

static void rz_trim_slash(char *path)
{
    size_t n = strlen(path);

    while (n > 0U && (path[n - 1U] == '/' || path[n - 1U] == '\\')) {
        path[n - 1U] = '\0';
        n--;
    }
}

static int ensure_dir(const char *path)
{
    FRESULT fr;

    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    fr = f_stat(path, NULL);
    if (fr == FR_OK) {
        return 0;
    }
    if (fr != FR_NO_FILE && fr != FR_NO_PATH) {
        printf("dest: mkdir check failed (%d)\r\n", (int)fr);
        return -1;
    }

    fr = f_mkdir(path);
    if (fr == FR_OK || fr == FR_EXIST) {
        return 0;
    }
    printf("dest: cannot create '%s' (%d)\r\n", path, (int)fr);
    return -1;
}

static int cmd_rz(int argc, char **argv)
{
    char dest[64];
    const char *def;
    int log_prev;
    int rc;

    def = (s_hooks_set && s_hooks.default_dir != NULL) ? s_hooks.default_dir : RZ_DEFAULT_DIR;

    if (argc >= 2) {
        if (strlen(argv[1]) >= sizeof(dest)) {
            printf("path too long\r\n");
            return 1;
        }
        strncpy(dest, argv[1], sizeof(dest));
        dest[sizeof(dest) - 1U] = '\0';
    } else {
        strncpy(dest, def, sizeof(dest));
        dest[sizeof(dest) - 1U] = '\0';
    }
    rz_trim_slash(dest);

    if (s_hooks.storage_ready != NULL && !s_hooks.storage_ready()) {
        printf("storage not ready\r\n");
        return 1;
    }
    if (s_hooks.before_io != NULL) {
        s_hooks.before_io();
    }

    if (ensure_dir(dest) != 0) {
        return 1;
    }

    cli_set_busy(true);
    log_prev = LOG_INFO;
    log_set_level(LOG_FATAL + 1);
    printf("ZMODEM receive → '%s' (host: sz <files>)\r\n", dest[0] != '\0' ? dest : "/");

    rc = zmodem_fatfs_receive(dest);

    log_set_level(log_prev);
    cli_set_busy(false);

    if (rc == 0 && s_hooks.after_receive != NULL) {
        s_hooks.after_receive();
    }
    printf("ZMODEM finished — use ls / cd.\r\n");
    return (rc == 0) ? 0 : 1;
}

void cli_zmodem_register(const cli_zmodem_hooks_t *hooks)
{
    static const cli_command_t rz_cmd = {
        "rz",
        "rz [dir] — ZMODEM receive to SD (default app)",
        cmd_rz,
    };
    static uint8_t done;

    if (done) {
        return;
    }
    done = 1U;

    if (hooks != NULL) {
        s_hooks = *hooks;
        s_hooks_set = 1U;
    }

    cli_register(&rz_cmd, 1U);
}
