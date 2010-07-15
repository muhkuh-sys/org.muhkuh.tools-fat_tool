
#include <string.h>
#include "fat/common.h"
#include "fat/cache.h"
#include "fat/file_allocation_table.h"

/*
Read numSectors sectors from a disc, starting at sector. 
numSectors is between 1 and 256
sector is from 0 to 2^28
buffer is a pointer to the memory to fill
*/
bool _FAT_disc_readSectors (const IO_INTERFACE *ptIo, u32 sector, u32 numSectors, void* buffer) 
{
	return ptIo->fn_readSectors(ptIo, sector, numSectors, buffer);
}

/*
Write numSectors sectors to a disc, starting at sector. 
numSectors is between 1 and 256
sector is from 0 to 2^28
buffer is a pointer to the memory to read from
*/
bool _FAT_disc_writeSectors (const IO_INTERFACE *ptIo, u32 sector, u32 numSectors, const void* buffer)
{
	return ptIo->fn_writeSectors(ptIo, sector, numSectors, buffer);
}

/*
Initialise the disc to a state ready for data reading or writing
*/
bool _FAT_disc_startup (const IO_INTERFACE *ptIo)
{
 return ptIo->fn_startup(ptIo);
}

bool _FAT_disc_isInserted (const IO_INTERFACE *ptIo)
{
  return ptIo->fn_isInserted(ptIo);
}

bool _FAT_disc_clearStatus (const IO_INTERFACE *ptIo)
{
  return ptIo->fn_clearStatus(ptIo);
}

/*
Put the disc in a state ready for power down.
Complete any pending writes and disable the disc if necessary
*/
bool _FAT_disc_shutdown (const IO_INTERFACE *ptIo)
{
  return ptIo->fn_shutdown(ptIo);
}

/*
Return a 32 bit value that specifies the capabilities of the disc
*/
u32 _FAT_disc_features (const IO_INTERFACE *ptIo)
{
  return ptIo->features;  
}
