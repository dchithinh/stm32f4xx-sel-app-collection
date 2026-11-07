#ifndef CLI_VFS_H
#define CLI_VFS_H

#include <stdbool.h>

/**
    * FatFs (microSD) shell commands: ls, pwd, cd, mkdir, rm, mv, df, cat, rescan.
    * Requires f_mount done by the app; use hooks for bus sharing / readiness / rescan.
    */
typedef struct {
    /** Called before each VFS op (e.g. stop I2S, switch SPI). May be NULL. */
    void (*before_io)(void);
    /** If NULL, storage is assumed ready. */
    bool (*storage_ready)(void);
    /**
    * If non-NULL, registers `rescan` (app-defined, e.g. refresh media index).
    * If NULL, no rescan command.
    */
    int (*rescan)(int argc, char **argv);
} cli_vfs_hooks_t;

/** Register VFS commands; pass hooks (copied). Call after cli_register_builtin(). */
void cli_vfs_register(const cli_vfs_hooks_t *hooks);

#endif
