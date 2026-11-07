/**
    * Lightweight interactive serial CLI (line editor + command dispatch).
    * Poll cli_poll() from the main loop; blocking commands (e.g. rz) set cli_is_busy().
    */
#ifndef CLI_H
#define CLI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int (*cli_cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    const char *help;
    cli_cmd_fn   fn;
} cli_command_t;

void cli_init(const char *prompt);
void cli_register(const cli_command_t *cmds, size_t count);
void cli_register_builtin(void);

/** Non-blocking: drains RX, assembles a line, runs matching command. */
void cli_poll(void);

bool cli_is_busy(void);
void cli_set_busy(bool busy);

/** Prints name + help for every command registered via cli_register / builtin. */
void cli_print_commands(void);

#endif
