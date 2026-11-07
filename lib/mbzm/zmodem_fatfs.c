/**
    * ZMODEM receive to FAT volume (mbzm), adapted from mbzm/rz.c.
    */
#include "zmodem_fatfs.h"

#include "ff.h"
#include "zmodem.h"

#include <stdio.h>
#include <string.h>

#define DATA_BUF_LEN 2048

static const char *s_dest_dir;
static char s_full_path[128];

static void strip_remote_path(char *name)
{
    char *base = name;

    for (char *p = name; *p != '\0'; p++) {
    if (*p == '/' || *p == '\\') {
        base = p + 1;
    }
    }
    if (base != name) {
    memmove(name, base, strlen(base) + 1U);
    }
}

static int build_target_path(const char *remote_name)
{
    strip_remote_path((char *)remote_name);

    if (s_dest_dir == NULL || s_dest_dir[0] == '\0') {
    return snprintf(s_full_path, sizeof(s_full_path), "%s", remote_name) >= (int)sizeof(s_full_path) ? -1 : 0;
    }
    return snprintf(s_full_path, sizeof(s_full_path), "%s/%s", s_dest_dir, remote_name) >= (int)sizeof(s_full_path) ? -1 : 0;
}

int zmodem_fatfs_receive(const char *dest_dir)
{
    uint8_t data_buf[DATA_BUF_LEN];
    uint16_t count;
    uint32_t received_data_size = 0;
    ZHDR hdr;
    FIL fil;
    uint8_t fil_open;
    int rc = -1;
    uint16_t result;

    s_dest_dir = (dest_dir != NULL) ? dest_dir : "";
    fil_open = 0U;

    printf("ZMODEM: on the host run e.g. sz <file> (lrzsz) at 115200 8N1\r\n");
    printf("Waiting for sender...\r\n");

    while (true) {
startframe:
    result = zm_await_header(&hdr);

    switch (result) {
    case CANCELLED:
        printf("ZMODEM cancelled\r\n");
        goto cleanup;
    case OK:
        switch (hdr.type) {
        case ZRQINIT:
        case ZEOF:
        result = zm_send_flags_hdr(ZRINIT, CANOVIO | CANFC32, 0, 0, 0);
        if (result == CANCELLED || result == CLOSED) {
            goto cleanup;
        }
        continue;

        case ZFIN:
        (void)zm_send_pos_hdr(ZFIN, 0);
        printf("ZMODEM done (%lu bytes this file)\r\n", (unsigned long)received_data_size);
        rc = 0;
        goto cleanup;

        case ZFILE: {
        count = DATA_BUF_LEN;
        result = zm_read_data_block(data_buf, &count);
        if (result == CANCELLED) {
            goto cleanup;
        }
        if (IS_ERROR(result)) {
            continue;
        }

        if (count >= DATA_BUF_LEN) {
            count = DATA_BUF_LEN - 1U;
        }
        data_buf[count] = '\0';
        if (build_target_path((char *)data_buf) != 0) {
            printf("path too long\r\n");
            goto cleanup;
        }
        /* No printf after ZRPOS — same UART as ZMODEM data. */
        if (fil_open) {
            (void)f_close(&fil);
            fil_open = 0U;
        }
        if (f_open(&fil, s_full_path, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
            printf("f_open failed\r\n");
            goto cleanup;
        }
        fil_open = 1U;
        received_data_size = 0U;

        result = zm_send_pos_hdr(ZRPOS, received_data_size);
        if (result == CANCELLED || result == CLOSED) {
            goto cleanup;
        }
        continue;
        }

        case ZDATA:
        while (true) {
            count = DATA_BUF_LEN;
            result = zm_read_data_block(data_buf, &count);

            if (!fil_open) {
            printf("ZDATA before file open\r\n");
            goto cleanup;
            }

            if (result == CANCELLED) {
            goto cleanup;
            }

            if (!IS_ERROR(result)) {
            UINT bw = 0;
            uint32_t payload = (uint32_t)count - 1U;

            if (payload > 0U) {
                if (f_write(&fil, data_buf, payload, &bw) != FR_OK || bw != payload) {
                printf("SD write error\r\n");
                goto cleanup;
                }
                received_data_size += payload;
            }

            if (result == GOT_CRCE) {
                break;
            }
            if (result == GOT_CRCG) {
                continue;
            }
            if (result == GOT_CRCQ) {
                result = zm_send_pos_hdr(ZACK, received_data_size);
                if (result == CANCELLED || result == CLOSED) {
                goto cleanup;
                }
                continue;
            }
            if (result == GOT_CRCW) {
                result = zm_send_pos_hdr(ZACK, received_data_size);
                if (result == CANCELLED || result == CLOSED) {
                goto cleanup;
                }
                break;
            }
            } else {
            result = zm_send_pos_hdr(ZRPOS, received_data_size);
            if (result == CANCELLED || result == CLOSED) {
                goto cleanup;
            }
            goto startframe;
            }
        }
        continue;

        default:
        continue;
        }
        break;

    case BAD_CRC:
    default:
        result = zm_send_pos_hdr(ZNAK, received_data_size);
        if (result == CANCELLED || result == CLOSED) {
        goto cleanup;
        }
        continue;
    }
    }

cleanup:
    if (fil_open) {
    (void)f_close(&fil);
    }
    return rc;
}
