#include "cli.h"

#include "debug_uart.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define CLI_LINE_MAX   96
#define CLI_ARGC_MAX   8

#define CLI_CMD_TABLES_MAX 8

static const char *s_prompt = "> ";
static const cli_command_t *s_tables[CLI_CMD_TABLES_MAX];
static size_t s_table_sizes[CLI_CMD_TABLES_MAX];
static size_t s_table_count;
static char s_line[CLI_LINE_MAX];
static unsigned s_line_len;
static unsigned s_drawn;
static bool s_need_prompt;
static bool s_busy;
static uint32_t s_ignore_cr_until;

static void cli_prompt(void)
{
    fputs(s_prompt, stdout);
    fflush(stdout);
    s_drawn = (unsigned)strlen(s_prompt);
}

static void cli_drop_escape(void)
{
    uint8_t b;

    if (!debug_uart_rx(&b, 30U)) {
        return;
    }
    if (b != '[' && b != 'O') {
        return;
    }
    while (debug_uart_rx(&b, 30U)) {
        if (b == '~' || (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z')) {
            break;
        }
    }
}

/*
 * pico_cli uses printf("\b \b"). ST-Link VCP often drops 0x08, so only the
 * space is shown. Redraw with CR (0x0D) instead: same as the CR in "\r\n".
 *
 * Tera Term: New-line Receive = CR (not LF / AUTO), Local echo OFF.
 */
static void cli_erase_one(void)
{
    unsigned plen = (unsigned)strlen(s_prompt);
    unsigned need;
    unsigned pad;
    uint8_t buf[CLI_LINE_MAX + 48];
    unsigned n = 0U;
    unsigned i;

    if (s_line_len == 0U) {
        return;
    }
    s_line_len--;
    s_line[s_line_len] = '\0';

    need = plen + s_line_len;
    pad = (s_drawn > need) ? (s_drawn - need) : 1U;

    buf[n++] = (uint8_t)'\r';
    memcpy(&buf[n], s_prompt, plen);
    n += plen;
    if (s_line_len > 0U) {
        memcpy(&buf[n], s_line, s_line_len);
        n += s_line_len;
    }
    for (i = 0; i < pad; i++) {
        buf[n++] = (uint8_t)' ';
    }
    buf[n++] = (uint8_t)'\r';
    memcpy(&buf[n], s_prompt, plen);
    n += plen;
    if (s_line_len > 0U) {
        memcpy(&buf[n], s_line, s_line_len);
        n += s_line_len;
    }
    debug_uart_write(buf, (uint16_t)n);
    s_drawn = need;
    s_ignore_cr_until = HAL_GetTick() + 15U;
}

static int cli_split(char *line, char *argv[CLI_ARGC_MAX])
{
    int argc = 0;
    char *p = line;

    while (*p != '\0' && argc < CLI_ARGC_MAX) {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        argv[argc++] = p;
        while (*p != '\0' && !isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        *p++ = '\0';
    }
    return argc;
}

static void cli_exec_line(char *line)
{
    char *argv[CLI_ARGC_MAX];
    int argc;
    size_t i;

    argc = cli_split(line, argv);
    if (argc == 0) {
        return;
    }

    for (size_t t = 0; t < s_table_count; t++) {
        for (i = 0; i < s_table_sizes[t]; i++) {
            if (strcmp(argv[0], s_tables[t][i].name) == 0) {
                (void)s_tables[t][i].fn(argc, argv);
                return;
            }
        }
    }

    printf("unknown command: %s (try help)\r\n", argv[0]);
}

void cli_init(const char *prompt)
{
    if (prompt != NULL && prompt[0] != '\0') {
        s_prompt = prompt;
    }
    s_line[0] = '\0';
    s_line_len = 0U;
    s_drawn = 0U;
    s_need_prompt = true;
    s_busy = false;
    s_ignore_cr_until = 0U;
}

void cli_register(const cli_command_t *cmds, size_t count)
{
    if (cmds == NULL || count == 0U) {
        return;
    }
    if (s_table_count >= CLI_CMD_TABLES_MAX) {
        return;
    }
    s_tables[s_table_count] = cmds;
    s_table_sizes[s_table_count] = count;
    s_table_count++;
}

bool cli_is_busy(void)
{
    return s_busy;
}

void cli_set_busy(bool busy)
{
    s_busy = busy;
}

void cli_print_commands(void)
{
    size_t t;
    size_t i;

    for (t = 0; t < s_table_count; t++) {
        for (i = 0; i < s_table_sizes[t]; i++) {
            printf("  %-10s %s\r\n", s_tables[t][i].name, s_tables[t][i].help);
        }
    }
}

void cli_poll(void)
{
    uint8_t c;

    if (s_busy) {
        return;
    }

    if (s_need_prompt) {
        cli_prompt();
        s_need_prompt = false;
    }

    while (debug_uart_try_rx(&c)) {
        if ((c == '\r' || c == '\n')
            && (int32_t)(HAL_GetTick() - s_ignore_cr_until) < 0) {
            continue;
        }

        if (c == '\r' || c == '\n') {
            printf("\r\n");
            fflush(stdout);
            if (s_line_len > 0U) {
                s_line[s_line_len] = '\0';
                cli_exec_line(s_line);
            }
            s_line_len = 0U;
            s_line[0] = '\0';
            s_drawn = 0U;
            s_need_prompt = true;
            break;
        }

        if (c == '\b' || c == 0x7FU) {
            cli_erase_one();
            continue;
        }

        if (c == 0x03U) {
            s_line_len = 0U;
            s_line[0] = '\0';
            printf("^C\r\n");
            fflush(stdout);
            s_drawn = 0U;
            s_need_prompt = true;
            break;
        }

        if (c == 0x1BU) {
            cli_drop_escape();
            continue;
        }

        if (!isprint((unsigned char)c)) {
            continue;
        }

        if (s_line_len + 1U >= CLI_LINE_MAX) {
            printf("\r\n(line too long)\r\n");
            fflush(stdout);
            s_line_len = 0U;
            s_drawn = 0U;
            s_need_prompt = true;
            break;
        }

        s_line[s_line_len++] = (char)c;
        s_line[s_line_len] = '\0';
        putchar((int)c);
        fflush(stdout);
        s_drawn = (unsigned)strlen(s_prompt) + s_line_len;
    }
}
