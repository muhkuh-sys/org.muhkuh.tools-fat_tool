/*
 cache dummy: gets pointers from a ram-based IO_INTERFACE and reads/writes directly from /to
 the image in memory. The IO interface is bypassed, because it does not offer partial sector reads/writes.
*/

#include <string.h>

#include "fat/common.h"
#include "fat/cache.h"
#include "fat/disk_io.h"

CACHE* _FAT_cache_constructor(CACHE* ptCache, const IO_INTERFACE* discInterface) {
	ptCache->disc = discInterface;
	return ptCache;
}

void _FAT_cache_destructor (CACHE* cache) 
{
}

/*
Reads some data from a cache page, determined by the sector number
  unsigned long           ulBlockSize;
  unsigned long           ulPagePerSecCnt;
  void                   *pvUser;
  unsigned long           ulStartOffset;
  unsigned long           ulDiskSize;
*/

bool _FAT_cache_readPartialSector (CACHE* cache, void* buffer, u32 sector, u32 offset, u32 size, u32 sectorsize) {
	char *pabBase = (char*) cache->disc->pvUser;
	u32 ulBlockSize = cache->disc->ulBlockSize;

	if (offset + size > sectorsize) {
		return false;
	}

	memcpy (buffer, pabBase + sector * ulBlockSize + offset, size);

	return true;
}

/* 
Writes some data to a cache page, making sure it is loaded into memory first.
*/
bool _FAT_cache_writePartialSector (CACHE* cache, const void* buffer, u32 sector, u32 offset, u32 size, u32 sectorsize) {
	char *pabBase = (char*) cache->disc->pvUser;
	u32 ulBlockSize = cache->disc->ulBlockSize;

	if (offset + size > sectorsize) {
		return false;
	}
		
	memcpy (pabBase + sector * ulBlockSize + offset, buffer, size);
	return true;
}

/* 
Writes some data to a cache page, zeroing out the page first
*/
bool _FAT_cache_eraseWritePartialSector (CACHE* cache, const void* buffer, u32 sector, u32 offset, u32 size, u32 sectorsize) {
	char *pabBase = (char*) cache->disc->pvUser;
	u32 ulBlockSize = cache->disc->ulBlockSize;

	if (offset + size > sectorsize) {
		return false;
	}

	memset (pabBase + sector * ulBlockSize, 0, ulBlockSize);
	memcpy (pabBase + sector * ulBlockSize + offset, buffer, size);

	return true;
}


bool _FAT_cache_flush (CACHE* cache) {
	return true;
}

void _FAT_cache_invalidate (CACHE* cache) {
}
