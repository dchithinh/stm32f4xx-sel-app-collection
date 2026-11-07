#ifndef ZMODEM_FATFS_H
#define ZMODEM_FATFS_H

/** Block until one or more files are received (or cancel). dest_dir may be "" or e.g. "app". */
int zmodem_fatfs_receive(const char *dest_dir);

#endif
