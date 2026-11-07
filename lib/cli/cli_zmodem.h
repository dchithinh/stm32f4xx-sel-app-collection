#ifndef CLI_ZMODEM_H
#define CLI_ZMODEM_H

#include <stdbool.h>

typedef struct {
    void (*before_io)(void);
    bool (*storage_ready)(void);
    /** Called after a successful receive session (e.g. rescan media). May be NULL. */
    void (*after_receive)(void);
    /** Used when `rz` has no argument (NULL → "app"). */
    const char *default_dir;
} cli_zmodem_hooks_t;

/** Register `rz [dir]` — ZMODEM receive to FatFs. Call after SD mount + cli_vfs_register(). */
void cli_zmodem_register(const cli_zmodem_hooks_t *hooks);

#endif
