
#include <string.h>

#include "fat/common.h"
#include "ramdisk/interface.h"
#include "fat/disk_io.h"


static int drv_ramdisk_readSectors (const struct IO_INTERFACE_STRUCT* ptIO, unsigned long sector, unsigned long numSectors, void* buffer); 
static int drv_ramdisk_writeSectors (const struct IO_INTERFACE_STRUCT* ptIO, unsigned long sector, unsigned long numSectors, const void* buffer); 
static int drv_ramdisk_startup (const struct IO_INTERFACE_STRUCT* ptIO); 
static int drv_ramdisk_isInserted (const struct IO_INTERFACE_STRUCT* ptIO); 
static int drv_ramdisk_clearStatus (const struct IO_INTERFACE_STRUCT* ptIO); 
static int drv_ramdisk_shutdown (const struct IO_INTERFACE_STRUCT* ptIO); 

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
  .ulDiskSize         = 0,

  .pfnErrorHandler    = NULL,
  .pfnvprintf         = NULL,
  .pvErrUser          = NULL

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
  0, 

  NULL,
  NULL,
  NULL
};
#endif


bool drv_ramdisk_checkBoundaries(const struct IO_INTERFACE_STRUCT* ptIO, unsigned long sector, unsigned long numSectors){
	unsigned long ulSectorSize = ptIO->ulBlockSize;
	unsigned long ulDiskSize = ptIO->ulDiskSize;

	if (sector * ulSectorSize > ulDiskSize || 
		(sector + numSectors) * ulSectorSize > ulDiskSize) {
		if (ptIO->pfnErrorHandler)
			ptIO->pfnErrorHandler(ptIO->pvErrUser, "drv_ramdisk_checkBoundaries: illegal sector access");
		return false;
	} else {
		return true;
	}
}

/*
Read numSectors sectors from a disc, starting at sector. 
numSectors is between 1 and 256
sector is from 0 to 2^28
buffer is a pointer to the memory to fill
*/
int drv_ramdisk_readSectors(const struct IO_INTERFACE_STRUCT* ptIO, unsigned long sector, unsigned long numSectors, void* buffer) 
{
  unsigned long ulSectorSize = ptIO->ulBlockSize;
  //unsigned long ulDiskSize = ptIO->ulDiskSize;
  unsigned char *pbData = (unsigned char*)ptIO->pvUser;

  if (drv_ramdisk_checkBoundaries(ptIO, sector, numSectors)){
  //if (sector * ulSectorSize < ulDiskSize && (sector + numSectors) * ulSectorSize < ulDiskSize){
	memcpy(buffer, pbData + sector * ulSectorSize, numSectors * ulSectorSize);
	return 1;
  } else {
	return 0;
  }
}

/*
Write numSectors sectors to a disc, starting at sector. 
numSectors is between 1 and 256
sector is from 0 to 2^28
buffer is a pointer to the memory to read from
*/
int drv_ramdisk_writeSectors(const struct IO_INTERFACE_STRUCT* ptIO, unsigned long sector, unsigned long numSectors, const void* buffer) 
{
  unsigned long ulSectorSize = ptIO->ulBlockSize;
  //unsigned long ulDiskSize = ptIO->ulDiskSize;
  unsigned char *pbData = (unsigned char*)ptIO->pvUser;

 // if (sector * ulSectorSize < ulDiskSize && (sector + numSectors) * ulSectorSize < ulDiskSize){
  if (drv_ramdisk_checkBoundaries(ptIO, sector, numSectors)){
	memcpy(pbData + sector * ulSectorSize, buffer, numSectors * ulSectorSize);
	return 1;
  }else {
	return 0;
  }
}

/*
Initialise the disc to a state ready for data reading or writing
*/
int drv_ramdisk_startup (const struct IO_INTERFACE_STRUCT* ptIO) 
{
  UNREFERENCED_PARAMETER(ptIO);

  return 1; 
}

int drv_ramdisk_isInserted (const struct IO_INTERFACE_STRUCT* ptIO) 
{
  UNREFERENCED_PARAMETER(ptIO);

  return 1;
}

int drv_ramdisk_clearStatus (const struct IO_INTERFACE_STRUCT* ptIO) 
{
  UNREFERENCED_PARAMETER(ptIO);

  return 1;
}

/*
Put the disc in a state ready for power down.
Complete any pending writes and disable the disc if necessary
*/
int drv_ramdisk_shutdown (const struct IO_INTERFACE_STRUCT* ptIO) 
{
  UNREFERENCED_PARAMETER(ptIO);

  return 1;
}
