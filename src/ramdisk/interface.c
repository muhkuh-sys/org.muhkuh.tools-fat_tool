
#include <string.h>

#include "compiler.h"
#include "ramdisk/interface.h"
#include "fat/disk_io.h"


static int drv_ramdisk_readSectors (const struct IO_INTERFACE_STRUCT* ptData, unsigned long sector, unsigned long numSectors, void* buffer); 
static int drv_ramdisk_writeSectors (const struct IO_INTERFACE_STRUCT* ptData, unsigned long sector, unsigned long numSectors, const void* buffer); 
static int drv_ramdisk_startup (const struct IO_INTERFACE_STRUCT* ptData); 
static int drv_ramdisk_isInserted (const struct IO_INTERFACE_STRUCT* ptData); 
static int drv_ramdisk_clearStatus (const struct IO_INTERFACE_STRUCT* ptData); 
static int drv_ramdisk_shutdown (const struct IO_INTERFACE_STRUCT* ptData); 

#ifdef __GNUC__
IO_INTERFACE g_tIoIfRamDisk =
{
  .ioType             = IO_TYPE_RAM,
  .features           = FEATURE_MEDIUM_CANREAD|FEATURE_MEDIUM_CANWRITE,
  .fn_startup         = drv_ramdisk_startup,
  .fn_isInserted      = drv_ramdisk_isInserted,
  .fn_readSectors     = drv_ramdisk_readSectors,
  .fn_writeSectors    = drv_ramdisk_writeSectors,
  .fn_clearStatus     = drv_ramdisk_clearStatus,
  .fn_shutdown        = drv_ramdisk_shutdown,
  .ulBlockSize        = 0,
  .pvUser             = NULL,
  .ulStartOffset      = 0,
  .ulDiskSize         = 0
};
#else
IO_INTERFACE g_tIoIfRamDisk =
{
  IO_TYPE_RAM,
  FEATURE_MEDIUM_CANREAD|FEATURE_MEDIUM_CANWRITE,
  drv_ramdisk_startup,
  drv_ramdisk_isInserted,
  drv_ramdisk_readSectors,
  drv_ramdisk_writeSectors,
  drv_ramdisk_clearStatus,
  drv_ramdisk_shutdown,
  0,
  NULL,
  0,
  0
};
#endif

/*
Read numSectors sectors from a disc, starting at sector. 
numSectors is between 1 and 256
sector is from 0 to 2^28
buffer is a pointer to the memory to fill
*/
int drv_ramdisk_readSectors(const struct IO_INTERFACE_STRUCT* ptData, unsigned long sector, unsigned long numSectors, void* buffer) 
{
  unsigned char *pbData;
  unsigned long ulSectorSize = ptData->ulBlockSize;


  /* cast void pointer to something with a size */ 
  pbData = (unsigned char*)ptData->pvUser;

  memcpy(buffer, pbData + sector * ulSectorSize, numSectors * ulSectorSize);

  return 1;
}

/*
Write numSectors sectors to a disc, starting at sector. 
numSectors is between 1 and 256
sector is from 0 to 2^28
buffer is a pointer to the memory to read from
*/
int drv_ramdisk_writeSectors(const struct IO_INTERFACE_STRUCT* ptData, unsigned long sector, unsigned long numSectors, const void* buffer) 
{
  unsigned char *pbData;
  unsigned long ulSectorSize = ptData->ulBlockSize;

  /* cast void pointer to something with a size */ 
  pbData = (unsigned char*)ptData->pvUser;

  memcpy(pbData + sector * ulSectorSize, buffer, numSectors * ulSectorSize);

  return 1;
}

/*
Initialise the disc to a state ready for data reading or writing
*/
int drv_ramdisk_startup (const struct IO_INTERFACE_STRUCT* ptData) 
{
  UNREFERENCED_PARAMETER(ptData);

  return 1; 
}

int drv_ramdisk_isInserted (const struct IO_INTERFACE_STRUCT* ptData) 
{
  UNREFERENCED_PARAMETER(ptData);

  return 1;
}

int drv_ramdisk_clearStatus (const struct IO_INTERFACE_STRUCT* ptData) 
{
  UNREFERENCED_PARAMETER(ptData);

  return 1;
}

/*
Put the disc in a state ready for power down.
Complete any pending writes and disable the disc if necessary
*/
int drv_ramdisk_shutdown (const struct IO_INTERFACE_STRUCT* ptData) 
{
  UNREFERENCED_PARAMETER(ptData);

  return 1;
}
