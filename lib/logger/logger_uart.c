/**
    * Glue for https://github.com/rxi/log.c — output via retargeted stdio (putchar -> UART).
    */
#include "logger_uart.h"

#include "stm32f4xx_hal.h"

#include <stdio.h>
#include <string.h>

static uint8_t s_inited;

static const char *log_basename(const char *path)
{
    const char *s = path;

    for (; *path != '\0'; path++) {
    if (*path == '/' || *path == '\\') {
        s = path + 1;
    }
    }
    return s;
}

static void log_uart_callback(log_Event *ev)
{
    char buf[224];
    int prefix;
    const char *fn = log_basename(ev->file);

    prefix = snprintf(buf, sizeof(buf), "[%08lu ms] %-5s %s:%d: ",
                    (unsigned long)HAL_GetTick(),
                    log_level_string(ev->level),
                    fn,
                    ev->line);
    if (prefix < 0 || (size_t)prefix >= sizeof(buf)) {
    return;
    }

    (void)vsnprintf(buf + (size_t)prefix, sizeof(buf) - (size_t)prefix, ev->fmt, ev->ap);

    for (const char *p = buf; *p != '\0'; p++) {
    (void)putchar((int)(unsigned char)*p);
    }
    (void)putchar('\r');
    (void)putchar('\n');
}

void logger_init_uart(void)
{
    if (s_inited) {
    return;
    }
    s_inited = 1U;

    log_set_quiet(true);
    log_set_level(LOGGER_DEFAULT_LEVEL);
    (void)log_add_callback(log_uart_callback, NULL, LOG_TRACE);
}
