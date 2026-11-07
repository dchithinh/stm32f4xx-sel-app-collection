/**
    * Init rxi/log.c on embedded: disables default stderr path (no time/strftime),
    * registers a callback that prints over the retargeted UART (putchar -> _write).
    */
#ifndef LOGGER_UART_H
#define LOGGER_UART_H

#include "log.h"

#ifndef LOGGER_DEFAULT_LEVEL
#define LOGGER_DEFAULT_LEVEL LOG_INFO
#endif

/** Call after debug_uart_init(); safe if called more than once (second call is no-op). */
void logger_init_uart(void);

#endif
