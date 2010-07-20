/*
 partition.c
 Functions for mounting and dismounting partitions
 on various block devices.

 Copyright (c) 2006 Michael "Chishm" Chisholm
	
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	2006-07-11 - Chishm
		* Original release

	2006-08-10 - Chishm
		* Fixed problem when openning files starting with "fat"
		
	2006-10-28 - Chishm
		* _partitions changed to _FAT_partitions to maintain the same style of naming as the functions

	2010-07-09   SL
	    * adapted to FAT utility on PC: partitions and caches are dynamically allocated

*/


#include "fat/partition.h"
#include "fat/bit_ops.h"
#include "fat/file_allocation_table.h"
#include "fat/directory.h"
#include "compiler.h"

#include <string.h>
#include <ctype.h>
#include <malloc.h>


/*
Data offsets
*/

// BIOS Parameter Block offsets
enum BPB {
	BPB_jmpBoot = 0x00,
	BPB_OEMName = 0x03,
	// BIOS Parameter Block
	BPB_bytesPerSector = 0x0B,
	BPB_sectorsPerCluster = 0x0D,
	BPB_reservedSectors = 0x0E,
	BPB_numFATs = 0x10,
	BPB_rootEntries = 0x11,
	BPB_numSectorsSmall = 0x13,
	BPB_mediaDesc = 0x15,
	BPB_sectorsPerFAT = 0x16,
	BPB_sectorsPerTrk = 0x18,
	BPB_numHeads = 0x1A,
	BPB_numHiddenSectors = 0x1C,
	BPB_numSectors = 0x20,
	// Ext BIOS Parameter Block for FAT16
	BPB_FAT16_driveNumber = 0x24,
	BPB_FAT16_reserved1 = 0x25,
	BPB_FAT16_extBootSig = 0x26,
	BPB_FAT16_volumeID = 0x27,
	BPB_FAT16_volumeLabel = 0x2B,
	BPB_FAT16_fileSysType = 0x36,
	// Bootcode
	BPB_FAT16_bootCode = 0x3E,
	// FAT32 extended block
	BPB_FAT32_sectorsPerFAT32 = 0x24,
	BPB_FAT32_extFlags = 0x28,
	BPB_FAT32_fsVer = 0x2A,
	BPB_FAT32_rootClus = 0x2C,
	BPB_FAT32_fsInfo = 0x30,
	BPB_FAT32_bkBootSec = 0x32,
	// Ext BIOS Parameter Block for FAT32
	BPB_FAT32_driveNumber = 0x40,
	BPB_FAT32_reserved1 = 0x41,
	BPB_FAT32_extBootSig = 0x42,
	BPB_FAT32_volumeID = 0x43,
	BPB_FAT32_volumeLabel = 0x47,
	BPB_FAT32_fileSysType = 0x52,
	// Bootcode
	BPB_FAT32_bootCode = 0x5A,
	BPB_bootSig_55 = 0x1FE,
	BPB_bootSig_AA = 0x1FF
};

#define MAXIMUM_CACHE_ENTRIES       3
 
static PARTITION* _FAT_partition_constructor ( const IO_INTERFACE* disc) {
	u32 ulSectorSize = disc->ulBlockSize;
	PARTITION* partition = (PARTITION*) malloc(sizeof(PARTITION));
	CACHE* ptCache = (CACHE*) malloc(sizeof(CACHE));
	//CACHE_ENTRY* patCacheEntries = (CACHE_ENTRY*) malloc(sizeof(CACHE_ENTRY) * MAXIMUM_CACHE_ENTRIES);
	//u8* pabCachePages = (u8*) malloc(ulSectorSize * MAXIMUM_CACHE_ENTRIES);

	if (partition == NULL || ptCache == NULL /* || patCacheEntries == NULL || pabCachePages == NULL*/) {
		free(partition);
		free(ptCache);
		//free(patCacheEntries);
		//free(pabCachePages);
		return NULL;
	}

	//ptCache->cacheEntries = patCacheEntries;
	//ptCache->numberOfPages = MAXIMUM_CACHE_ENTRIES;
	//ptCache->pages = pabCachePages;
	ptCache->pageSize = ulSectorSize;
	ptCache->pvUser = NULL;

	partition->cache = _FAT_cache_constructor(ptCache, disc);
	partition->disc = (IO_INTERFACE*) disc;
	return partition;

}


static void _FAT_partition_destructor (PARTITION* ptPartition) 
{
	if (ptPartition!=NULL) {
		//free(ptPartition->cache->cacheEntries);
		//free(ptPartition->cache->pages);
		free(ptPartition->cache);
		free(ptPartition);
	}
}

/*
	get the sector size and number of sectors from a FAT12/16/32 boot sector
	in:
	pvImage, sizImage: Pointer to and size of partition image
	sizPartitionOffset: start of the partition
	out:
    psizBytesPerSector
	psizNumberOfSectors
	returns true if successful, false otherwise
*/
bool _FAT_partition_recognize ( void* pvImage, size_t sizImage, size_t sizPartitionOffset, 
									size_t *psizBytesPerSector, size_t *psizNumberOfSectors) {
	u8 *sectorBuffer = (u8*)pvImage + sizPartitionOffset;
	u32 ulNumberOfSectors;
	u16 ulBytesPerSector;
	if (sizImage<sizPartitionOffset || sizImage-sizPartitionOffset < 256) return false;

	if ((sectorBuffer[0x36] == 'F') && (sectorBuffer[0x37] == 'A') && (sectorBuffer[0x38] == 'T') ||
		(sectorBuffer[0x52] == 'F') && (sectorBuffer[0x53] == 'A') && (sectorBuffer[0x54] == 'T')) {
		ulNumberOfSectors = (u16) u8array_to_u16( sectorBuffer, BPB_numSectorsSmall); 
		if (ulNumberOfSectors == 0) {
			ulNumberOfSectors = u8array_to_u32( sectorBuffer, BPB_numSectors);	
		}

		ulBytesPerSector = u8array_to_u16(sectorBuffer, BPB_bytesPerSector);

		if (ulNumberOfSectors > 0 ||
			ulBytesPerSector > 0 ||
			(size_t) ulNumberOfSectors * ulBytesPerSector <= sizImage) {
				*psizBytesPerSector = (size_t) ulBytesPerSector;
				*psizNumberOfSectors = (size_t) ulNumberOfSectors;
				return true;
		}
	}

	return false;

}

/* partition.cache and partition.disc_io must be defined */
static bool _FAT_partition_mount ( PARTITION* partition) {
	u32 i;
	u32 bootSector;
	u8 sectorBuffer[EXT_CACHE_PAGE_SIZE];
	u32 ulRootDirSectorSize;

	memset(sectorBuffer, 0, sizeof(sectorBuffer));

	// Read first sector of disc
	if ( !_FAT_disc_readSectors (partition->disc, 0, 1, sectorBuffer)) {
		return false;
	}

	// Make sure it is a valid MBR or boot sector
	if ( (sectorBuffer[BPB_bootSig_55] != 0x55) || (sectorBuffer[BPB_bootSig_AA] != 0xAA)) {
		return false;
	}

	// Check if there is a FAT string, which indicates this is a boot sector
	if ((sectorBuffer[0x36] == 'F') && (sectorBuffer[0x37] == 'A') && (sectorBuffer[0x38] == 'T')) {
		bootSector = 0;
	} else if ((sectorBuffer[0x52] == 'F') && (sectorBuffer[0x53] == 'A') && (sectorBuffer[0x54] == 'T')) {
		// Check for FAT32
		bootSector = 0;
	} else {
		// This is an MBR
		// Find first valid partition from MBR
		// First check for an active partition
		for (i=0x1BE; (i < 0x1FE) && (sectorBuffer[i] != 0x80); i+= 0x10);
		// If it didn't find an active partition, search for any valid partition
		if (i == 0x1FE) {
			for (i=0x1BE; (i < 0x1FE) && (sectorBuffer[i+0x04] == 0x00); i+= 0x10);
		}
		
		// Go to first valid partition
		if ( i != 0x1FE) {
			// Make sure it found a partition
			bootSector = u8array_to_u32(sectorBuffer, 0x8 + i);
		} else {
			bootSector = 0;	// No partition found, assume this is a MBR free disk
		}
	}

	// Read in boot sector
	if ( !_FAT_disc_readSectors (partition->disc, bootSector, 1, sectorBuffer)) 
    {
		return false;
	}

	partition->fMounted = true;

	// Store required information about the file system
	partition->fat.sectorsPerFat = u8array_to_u16(sectorBuffer, BPB_sectorsPerFAT);
	if (partition->fat.sectorsPerFat == 0) {
		partition->fat.sectorsPerFat = u8array_to_u32( sectorBuffer, BPB_FAT32_sectorsPerFAT32); 
	}

	partition->numberOfSectors = u8array_to_u16( sectorBuffer, BPB_numSectorsSmall); 
	if (partition->numberOfSectors == 0) {
		partition->numberOfSectors = u8array_to_u32( sectorBuffer, BPB_numSectors);	
	}

	partition->bytesPerSector = u8array_to_u16(sectorBuffer, BPB_bytesPerSector);	// Sector size is redefined to be 512 bytes
	partition->sectorsPerCluster = sectorBuffer[BPB_sectorsPerCluster] * u8array_to_u16(sectorBuffer, BPB_bytesPerSector) / partition->bytesPerSector;
	partition->bytesPerCluster = partition->bytesPerSector * partition->sectorsPerCluster;
	partition->fat.fatStart = bootSector + u8array_to_u16(sectorBuffer, BPB_reservedSectors); 

	partition->rootDirStart = partition->fat.fatStart + (sectorBuffer[BPB_numFATs] * partition->fat.sectorsPerFat);
	
	ulRootDirSectorSize = u8array_to_u16(sectorBuffer, BPB_rootEntries) * DIR_ENTRY_DATA_SIZE;
	ulRootDirSectorSize = (ulRootDirSectorSize + partition->bytesPerSector - 1) / partition->bytesPerSector;
	
	partition->dataStart = partition->rootDirStart + ulRootDirSectorSize;

	partition->totalSize = (partition->numberOfSectors - partition->dataStart) * partition->bytesPerSector;

	// Store info about FAT
	partition->fat.lastCluster = (partition->numberOfSectors - partition->dataStart) / partition->sectorsPerCluster;
	partition->fat.firstFree = CLUSTER_FIRST;

	if (partition->fat.lastCluster < CLUSTERS_PER_FAT12) {
		partition->filesysType = FS_FAT12;	// FAT12 volume
	} else if (partition->fat.lastCluster < CLUSTERS_PER_FAT16) {
		partition->filesysType = FS_FAT16;	// FAT16 volume
	} else {
		partition->filesysType = FS_FAT32;	// FAT32 volume
	}

	if (partition->filesysType != FS_FAT32) {
		partition->rootDirCluster = FAT16_ROOT_DIR_CLUSTER;
	} else {
		// Set up for the FAT32 way
		partition->rootDirCluster = u8array_to_u32(sectorBuffer, BPB_FAT32_rootClus); 
		// Check if FAT mirroring is enabled
		if (!(sectorBuffer[BPB_FAT32_extFlags] & 0x80)) {
			// Use the active FAT
			partition->fat.fatStart = partition->fat.fatStart + ( partition->fat.sectorsPerFat * (sectorBuffer[BPB_FAT32_extFlags] & 0x0F));
		}
	}

	// Set current directory to the root
	partition->cwdCluster = partition->rootDirCluster;
	
	// Check if this disc is writable, and set the readOnly property appropriately
	partition->readOnly = !(_FAT_disc_features(partition->disc) & FEATURE_MEDIUM_CANWRITE);
	
	// There are currently no open files on this partition
	partition->openFileCount = 0;
  
	return true;
}



PARTITION* _FAT_partition_mountCustomInterface(const IO_INTERFACE* device, u32 cacheSize) {

	PARTITION* ptPartition = _FAT_partition_constructor (device);
	
	if (ptPartition != NULL) {
		if (!_FAT_partition_mount(ptPartition)) {
			_FAT_disc_shutdown (device); 
			_FAT_partition_destructor (ptPartition);
			ptPartition = NULL;
		}
	}

	return ptPartition;
}

bool _FAT_partition_unmount(PARTITION* ptPartition) 
{
	if (ptPartition == NULL) {
		return false;
	}

	if(ptPartition->openFileCount > 0) {
		// There are still open files that need closing
		return false;
	}

	_FAT_disc_shutdown (ptPartition->disc); 
	_FAT_partition_destructor (ptPartition);
	
	return true;
}

bool _FAT_partition_unsafeUnmount(PARTITION* ptPartition) 
{
	if (ptPartition == NULL) {
		return false;
	}
	
	_FAT_disc_shutdown (ptPartition->disc); 
	_FAT_cache_invalidate (ptPartition->cache);
	_FAT_partition_destructor (ptPartition);
	return true;
}

/*
PARTITION* _FAT_partition_getPartitionFromPath (const char* path) 
{
	UNREFERENCED_PARAMETER(path);
	// Incorrect device name
	return NULL;
}
*/