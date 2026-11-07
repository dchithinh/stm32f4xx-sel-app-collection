/**
    * Common FatFs CLI — microSD or any mounted FatFs volume "".
    */
#include "cli_vfs.h"
#include "cli.h"

#include "ff.h"

#include <stdio.h>
#include <string.h>

static cli_vfs_hooks_t s_hooks;
static uint8_t s_hooks_set;

/** Trim leading/trailing spaces and tabs (in place). */
static void vfs_trim_path(char *s)
{
    char *start = s;
    char *end;

    if (s == NULL || s[0] == '\0') {
        return;
    }
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1U);
    }
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t')) {
        end--;
    }
    *end = '\0';
}

static void vfs_print_name(const char *raw)
{
    char name[FF_LFN_BUF + 4];

    if (raw == NULL) {
        return;
    }
    strncpy(name, raw, sizeof(name) - 1U);
    name[sizeof(name) - 1U] = '\0';
    vfs_trim_path(name);
    printf("%s", name);
}

static void vfs_before(void)
{
    if (s_hooks.before_io != NULL) {
    s_hooks.before_io();
    }
}

static int vfs_ready_or_fail(void)
{
    if (s_hooks.storage_ready != NULL && !s_hooks.storage_ready()) {
    printf("storage not ready\r\n");
    return -1;
    }
    return 0;
}

static const char *fr_str(FRESULT fr)
{
    switch (fr) {
    case FR_OK: return "OK";
    case FR_DISK_ERR: return "disk error";
    case FR_NOT_READY: return "not ready";
    case FR_NO_FILE: return "no file";
    case FR_NO_PATH: return "no path";
    case FR_INVALID_NAME: return "invalid name";
    case FR_DENIED: return "denied";
    case FR_EXIST: return "exists";
    case FR_INVALID_OBJECT: return "invalid object";
    case FR_WRITE_PROTECTED: return "write protected";
    case FR_NOT_ENOUGH_CORE: return "no memory";
    case FR_TOO_MANY_OPEN_FILES: return "too many open files";
    default: return "error";
    }
}

static int cmd_ls(int argc, char **argv)
{
    char path_buf[FF_LFN_BUF + 4];
    const char *path = ".";

    if (argc >= 2) {
        strncpy(path_buf, argv[1], sizeof(path_buf) - 1U);
        path_buf[sizeof(path_buf) - 1U] = '\0';
        vfs_trim_path(path_buf);
        path = path_buf;
    }
    DIR dir;
    FILINFO info;
    FRESULT fr;

    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();

    fr = f_opendir(&dir, path);
    if (fr != FR_OK) {
    printf("ls: '%s' — %s (%d)\r\n", path, fr_str(fr), (int)fr);
    return 1;
    }

    printf("'%s':\r\n", path);
    while (true) {
    fr = f_readdir(&dir, &info);
    if (fr != FR_OK || info.fname[0] == '\0') {
        break;
    }
        if (info.fattrib & AM_DIR) {
            printf("  d  ");
            vfs_print_name(info.fname);
            printf("\r\n");
        } else {
            printf("  %10lu  ", (unsigned long)info.fsize);
            vfs_print_name(info.fname);
            printf("\r\n");
        }
    }
    (void)f_closedir(&dir);
    return 0;
}

static int cmd_pwd(int argc, char **argv)
{
    char path[FF_LFN_BUF + 4];

    (void)argc;
    (void)argv;
    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();

    if (f_getcwd(path, sizeof(path)) != FR_OK) {
    printf("pwd failed\r\n");
    return 1;
    }
    printf("%s\r\n", path);
    return 0;
}

static int cmd_cd(int argc, char **argv)
{
    char path_buf[FF_LFN_BUF + 4];
    const char *path = "/";
    FRESULT fr;

    if (vfs_ready_or_fail() != 0) {
        return 1;
    }
    if (argc >= 2) {
        strncpy(path_buf, argv[1], sizeof(path_buf) - 1U);
        path_buf[sizeof(path_buf) - 1U] = '\0';
        vfs_trim_path(path_buf);
        path = path_buf;
    }
    vfs_before();

    fr = f_chdir(path);
    if (fr != FR_OK) {
    printf("cd: %s (%d)\r\n", fr_str(fr), (int)fr);
    return 1;
    }
    return cmd_pwd(0, NULL);
}

static int cmd_mkdir(int argc, char **argv)
{
    FRESULT fr;

    if (argc < 2) {
    printf("usage: mkdir <path>\r\n");
    return 1;
    }
    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();

    vfs_trim_path(argv[1]);
    if (argv[1][0] == '\0') {
        printf("usage: mkdir <path>\r\n");
        return 1;
    }

    fr = f_mkdir(argv[1]);
    if (fr != FR_OK) {
        printf("mkdir: %s (%d)\r\n", fr_str(fr), (int)fr);
        return 1;
    }
    return 0;
}

static int cmd_rm(int argc, char **argv)
{
    FRESULT fr;

    if (argc < 2) {
    printf("usage: rm <path>\r\n");
    return 1;
    }
    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();

    vfs_trim_path(argv[1]);
    if (argv[1][0] == '\0') {
        printf("usage: rm <path>\r\n");
        return 1;
    }

    fr = f_unlink(argv[1]);
    if (fr != FR_OK) {
        printf("rm: %s (%d)\r\n", fr_str(fr), (int)fr);
        return 1;
    }
    return 0;
}

static int cmd_mv(int argc, char **argv)
{
    FRESULT fr;

    if (argc < 3) {
    printf("usage: mv <from> <to>\r\n");
    return 1;
    }
    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();

    vfs_trim_path(argv[1]);
    vfs_trim_path(argv[2]);
    if (argv[1][0] == '\0' || argv[2][0] == '\0') {
        printf("usage: mv <from> <to>\r\n");
        return 1;
    }

    fr = f_rename(argv[1], argv[2]);
    if (fr != FR_OK) {
        printf("mv: %s (%d)\r\n", fr_str(fr), (int)fr);
        return 1;
    }
    return 0;
}

static int cmd_df(int argc, char **argv)
{
    FATFS *fs;
    DWORD free_clust;
    FRESULT fr;

    (void)argc;
    (void)argv;
    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();

    fr = f_getfree("", &free_clust, &fs);
    if (fr != FR_OK) {
    printf("df: %s\r\n", fr_str(fr));
    return 1;
    }
    {
    DWORD total = (fs->n_fatent - 2U) * fs->csize;
    DWORD freeb = free_clust * fs->csize;

    printf("free %lu / total %lu sectors (~%lu KiB free)\r\n",
            (unsigned long)freeb,
            (unsigned long)total,
            (unsigned long)((freeb * 512UL) / 1024UL));
    }
    return 0;
}

static int cmd_cat(int argc, char **argv)
{
    FIL fp;
    char buf[65];
    UINT br;
    FRESULT fr;
    UINT total = 0;

    if (argc < 2) {
    printf("usage: cat <file> (max 4 KiB)\r\n");
    return 1;
    }
    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();

    vfs_trim_path(argv[1]);
    if (argv[1][0] == '\0') {
        printf("usage: cat <file> (max 4 KiB)\r\n");
        return 1;
    }

    fr = f_open(&fp, argv[1], FA_READ);
    if (fr != FR_OK) {
    printf("cat: %s\r\n", fr_str(fr));
    return 1;
    }

    while (total < 4096U) {
    fr = f_read(&fp, buf, sizeof(buf) - 1U, &br);
    if (fr != FR_OK || br == 0U) {
        break;
    }
    buf[br] = '\0';
    fputs(buf, stdout);
    total += br;
    }
    fputs("\r\n", stdout);
    (void)f_close(&fp);
    if (total >= 4096U) {
    printf("... (truncated)\r\n");
    }
    return 0;
}

static int cmd_rescan_dispatch(int argc, char **argv)
{
    if (s_hooks.rescan == NULL) {
    printf("rescan not configured\r\n");
    return 1;
    }
    if (vfs_ready_or_fail() != 0) {
    return 1;
    }
    vfs_before();
    return s_hooks.rescan(argc, argv);
}

void cli_vfs_register(const cli_vfs_hooks_t *hooks)
{
    static const cli_command_t base[] = {
    { "ls",    "ls [path] — list directory (default .)",     cmd_ls },
    { "pwd",   "pwd — current working directory",            cmd_pwd },
    { "cd",    "cd [path] — change dir (default /)",         cmd_cd },
    { "mkdir", "mkdir <path> — create directory",            cmd_mkdir },
    { "rm",    "rm <path> — delete file or empty directory", cmd_rm },
    { "mv",    "mv <from> <to> — rename",                    cmd_mv },
    { "df",    "df — free space",                              cmd_df },
    { "cat",   "cat <file> — print text (<= 4 KiB)",          cmd_cat },
    };

    if (hooks != NULL) {
    s_hooks = *hooks;
    s_hooks_set = 1U;
    }

    cli_register(base, sizeof(base) / sizeof(base[0]));

    if (s_hooks_set && s_hooks.rescan != NULL) {
    static const cli_command_t resc = {
        "rescan",
        "rescan — app hook (e.g. refresh media index)",
        cmd_rescan_dispatch,
    };
    cli_register(&resc, 1U);
    }
}
