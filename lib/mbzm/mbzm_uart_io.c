/**
    * ZMODEM UART I/O. Do not include ztypes.h here: its CR/CRC macros clash with CMSIS.
    */
#include "debug_uart.h"

#include <stdint.h>

typedef uint16_t ZRESULT;

#define ZM_OK     0x0100U
#define ZM_CLOSED 0x2000U

ZRESULT zm_recv(void)
{
    uint8_t c;

    if (debug_uart_rx(&c, 60000U) != 0) {
    return (ZRESULT)c;
    }
    return ZM_CLOSED;
}

ZRESULT zm_send(uint8_t chr)
{
    debug_uart_tx_byte(chr);
    return ZM_OK;
}
