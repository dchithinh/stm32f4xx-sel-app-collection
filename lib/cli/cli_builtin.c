#include "cli.h"

#include "stm32f4xx_hal.h"

#include <stdio.h>
#include <string.h>

static int cmd_help(int argc, char **argv);
static int cmd_echo(int argc, char **argv);
static int cmd_uptime(int argc, char **argv);

static const cli_command_t s_builtin[] = {
    { "help",   "help [cmd] — list commands",        cmd_help },
    { "echo",   "echo <text...>",                    cmd_echo },
    { "uptime", "uptime — milliseconds since boot",  cmd_uptime },
};

static int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Commands:\r\n");
    cli_print_commands();
    return 0;
}

static int cmd_echo(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (i > 1) {
            putchar(' ');
        }
        fputs(argv[i], stdout);
    }
    fputs("\r\n", stdout);
    return 0;
}

static int cmd_uptime(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("uptime %lu ms\r\n", (unsigned long)HAL_GetTick());
    return 0;
}

void cli_register_builtin(void)
{
    cli_register(s_builtin, sizeof(s_builtin) / sizeof(s_builtin[0]));
}
