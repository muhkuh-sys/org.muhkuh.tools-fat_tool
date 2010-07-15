#ifndef RAMDISK_INTERFACE_H_
#define RAMDISK_INTERFACE_H_

#include "fat/disk_io.h"

#define RAMDISK_SIZE          0x00100000UL
#define RAMDISK_CRC32_LENGTH  0x400UL

extern IO_INTERFACE g_tIoIfRamDisk;

#endif /*RAMDISK_INTERFACE_H_*/
