/*****************************************************************************
 
   Copyright (c) Hilscher GmbH. All Rights Reserved.
 
 *****************************************************************************
 
   Filename:
    $Workfile: format.c $
   Last Modification:
    $Author: Michaelt $
    $Modtime: 23.03.07 8:08 $
    $Revision: 1 $
   
   Targets:
     O/S independent  : yes
 
   Description:
     FAT formatting routines
       
   Changes:
 
     Version   Date        Author   Description
     -------------------------------------------------------------------------
      2        13.09.2007  MT       Change:
                                      - Calculation of block/sector size moved to
                                        seperated function to be usable in main
      1        03.04.2007  CT       initial version
 
******************************************************************************/

/*****************************************************************************/
/*! \file format.c                                                    
*   FAT formatting routines                                                  */
/*****************************************************************************/

/*****************************************************************************/
/*! \addtogroup 2ND_STAGE_FAT
    \{                                                                       */
/*****************************************************************************/

#include "compiler.h"
#include "fat/format.h"
#include "fat/partition.h"
//#include "serflash/Drv_SpiFlash.h"
#include <string.h> /* memcpy/memset */


static const unsigned char abBootSector0[11] =
  {
    0xeb, 0x3c, 0x90,                   /* BS_jmpBoot */
    'H', 'I', 'L', 'S', 'C', 'H', 'E', 'R'        /* BS_OEMName */
  };

typedef union
{
  uint8_t  ab[EXT_CACHE_PAGE_SIZE];
  uint16_t us[EXT_CACHE_PAGE_SIZE / sizeof(uint16_t)];
  uint32_t ul[EXT_CACHE_PAGE_SIZE / sizeof(uint32_t)];
  fat_bootsec_t tFat;
  fat_direntry_t atDir[EXT_CACHE_PAGE_SIZE / sizeof(fat_direntry_t)];
  
} FAT_BOOTSECTOR_U;

/*****************************************************************************/
/*! Initialize the FAT for the partition
 *   \param ptPartition Partition to initialize 
 *   \return !=0 on success                                                  */
/*****************************************************************************/
static int initFat(PARTITION *ptPartition, FAT_BOOTSECTOR_U* puBootSec)
{
  int iResult;
  const IO_INTERFACE *ptIo;
  FS_TYPE tFatType;
  unsigned long ulFatStartSector;
  unsigned long ulFatSizeInSectors;
  unsigned long ulBytesPerSector;

  ptIo = ptPartition->disc;
  tFatType = ptPartition->filesysType;
  ulBytesPerSector = ptPartition->bytesPerSector;
  ulFatSizeInSectors = ptPartition->fat.sectorsPerFat;
  ulFatStartSector = ptPartition->fat.fatStart;

  /* check parameters */
  if ( ulFatSizeInSectors == 0 )
  {
    /* the fat must have at least one sector */
    return (1 == 0);
  }

  /* assume success */
  iResult = (1 == 1);

  /* init the fat */
  memset(puBootSec->ab, 0, ulBytesPerSector);

  /* the first fat entry must contain the media type in the lower 8 bits,
   * the second entry must be EOC */
  switch (tFatType)
  {
    case FS_FAT12:
      puBootSec->ul[0] = 0x0ff8 | (0x0fff << 12);
      break;

    case FS_FAT16:
      puBootSec->ul[0] = 0xfff8 | (0xffff << 16);
      break;

    case FS_FAT32:
      puBootSec->ul[0] = 0x0ffffff8;
      puBootSec->ul[1] = 0x0fffffff;
      break;

    default:
      iResult = (1 == 0);
  }

  if ( iResult )
  {
    iResult = ptIo->fn_writeSectors(ptIo, ulFatStartSector, 1, puBootSec->ab);
    if ( iResult )
    {
      /* clear the fat buffer */
      memset(puBootSec->ab, 0, ulBytesPerSector);

      /* loop over all other fat sectors and clear them */
      while ( --ulFatSizeInSectors )
      {
        iResult = ptIo->fn_writeSectors(ptIo, ++ulFatStartSector, 1, puBootSec->ab);
        if ( !iResult )
        {
          break;
        }
      };
    }
  }

  return iResult;
}

/*****************************************************************************/
/*! Initialize root directory for partition
 *   \param ptPartition        Partition to initialize 
 *   \param ulTotalDirEntries  Maximum number of directory entries
 *   \return !=0 on success                                                  */ 
/*****************************************************************************/
static int initRootDir(PARTITION* ptPartition, FAT_BOOTSECTOR_U* puBootSec, unsigned long ulTotalDirEntries)
{
  int iResult;
  const IO_INTERFACE *ptIo;
  FS_TYPE tFatType;
  unsigned long ulDirStartSector;
  unsigned long ulDirEntriesPerSector;
  unsigned long ulDirEntryCnt;
  unsigned long ulBytesPerSector;


  ptIo = ptPartition->disc;
  tFatType = ptPartition->filesysType;
  ulDirStartSector = ptPartition->rootDirCluster * ptPartition->sectorsPerCluster + ptPartition->rootDirStart;
  ulBytesPerSector = ptPartition->bytesPerSector;
  ulDirEntriesPerSector = ulBytesPerSector / sizeof(fat_direntry_t);

  /* init the dir sector */
  memset(puBootSec->ab, 0, ulBytesPerSector);

  /* construct root dir entry */
  memcpy(puBootSec->atDir[0].name, "TESTIMAGE01", 11);

  /* set attributes */
  puBootSec->atDir[0].attr = FatDirAttr_VolumeID;

  /* write first root dir block */
  iResult = ptIo->fn_writeSectors(ptIo, ulDirStartSector, 1, puBootSec->ab);
  if ( iResult )
  {
    /* clear the dir buffer */
    memset(puBootSec->ab, 0, ulBytesPerSector);
    /* processed one complete dir sector */
    ulDirEntryCnt = ulDirEntriesPerSector;

    /* some other dir sectors left? */
    while ( ulDirEntryCnt < ulTotalDirEntries )
    {
      iResult = ptIo->fn_writeSectors(ptIo, ++ulDirStartSector, 1, puBootSec->ab);
      if ( !iResult )
      {
        break;
      }
      ulDirEntryCnt += ulDirEntriesPerSector;
    };
  }

  return iResult;
}

/*****************************************************************************/
/*! Calculates the sector size which should be used for the device.
 *  NOTE: Automatically inserts the value into the ptIo structure
 *    \param ptIo I/O Interface to use
 *    \return !=0 on success                                                 */
/*****************************************************************************/

#if 0
unsigned long CalculateSectorSize(IO_INTERFACE *ptIo)
{
  unsigned long ulRet = 0;
  
  switch ( ptIo->ioType )
  {
    case IO_TYPE_SPI:
      if(ptIo->ulBlockSize != 0)
      {
        ulRet = ptIo->ulBlockSize;
      }
      else
      {
        SPI_FLASH_T* ptFlash = (SPI_FLASH_T*)ptIo->pvUser;
        /* does the device support page erase? */
        if( (ptFlash->tAttributes.bErasePageOpcode|ptFlash->tAttributes.bEraseAndPageProgOpcode)!=0 )
        {
          /* use flash page size as blocksize */
          ulRet = ptFlash->tAttributes.ulPageSize;
        }
        else if( ptFlash->tAttributes.bEraseSectorOpcode!=0 )
        {
          ulRet = ptFlash->tAttributes.ulPageSize * ptFlash->tAttributes.ulSectorPages; 
        }

        ptIo->ulPagePerSecCnt = 1;
        while(ulRet < 512)
        {
          ulRet *= 2;
          ++ptIo->ulPagePerSecCnt;
        }

        ptIo->ulBlockSize = ulRet;
      }
      break;

    case IO_TYPE_RAM:
    case IO_TYPE_PARFLASH:
      /* ram and parflash are fixed to 512 bytes */
      if(0 != ptIo->ulBlockSize)
      { 
        ulRet = ptIo->ulBlockSize;
      } else
      {
        ulRet     = 512;
        ptIo->ulBlockSize = ulRet;
      }
      break;

    default:
      /* 0 triggers an error */
      ulRet = 0;
      break;
  };

  /* check if the block size fits into the cache */
  if( ptIo->ulBlockSize> EXT_CACHE_PAGE_SIZE )
  {
    /* the block size does not fit into the cache -> error */
    ulRet = 0;
  }

  return ulRet;
}
#endif

/*****************************************************************************/
/*! Format a partition
 *    \param ptIo I/O Interface to use for format
 *    \return !=0 on success                                                 */
/*****************************************************************************/
int formatFat(IO_INTERFACE *ptIo)
{
  unsigned int uiBytesPerSec;
  FS_TYPE tFatType;
  unsigned long ulPartitionSize;
  unsigned long ulPartitionSectors;
  unsigned long ulNormedPartitionSize;
  unsigned long ulFatControlledSectors;
  unsigned long ulReservedSectors;
  unsigned long ulRootDirEntries;
  unsigned long ulRootDirSectors;
  unsigned long ulSectorsPerCluster;
  unsigned long ulClusterSize;
  unsigned long ulNumberOfClusters;
  unsigned long ulMaxFatClusters;
  unsigned long ulMinFatClusters;
  unsigned long ulFatSizeSectors;
  unsigned long ulClusterCnt;
  unsigned long ulSectorCnt;
  unsigned long ulFatBitCount;
  unsigned int uiFatElementSize;
  fat16_bootsec_t fatId;
  int iResult;
  unsigned long ulFirstRootDirSector;
  PARTITION tPartition;
  FAT_BOOTSECTOR_U uBootSec;

  /* get the bytes per sector */
  uiBytesPerSec = ptIo->ulBlockSize; 
  //CalculateSectorSize(ptIo);

  /* check bytes per sector, must not be 0 */
  if ( uiBytesPerSec == 0 )
  {
    /* error, can't format partition */
    return (1 == 0);
  }

  /* get the total size of the partition in bytes */
  ulPartitionSize = ptIo->ulDiskSize;
  /* get the number of sectors of the partition */
  ulPartitionSectors = ulPartitionSize / uiBytesPerSec;

  /* MS FAT specification describes which FAT Type should be used for 
     different partition sizes, but this is normed to 512 bytes.
     So we use normalized Partitionsize by dividing it by (BytesPerSec / 512) */
  ulNormedPartitionSize = ulPartitionSize / (uiBytesPerSec / 512);
  
  /* get the usual fat type for the size */
  if ( ulNormedPartitionSize <= 4300800 )
  {
    /* up to 4.1MB is FAT12 */
    tFatType = FS_FAT12;
  }
  else if ( ulNormedPartitionSize <= 34099200 )
  {
    /* up to 32.5MB is FAT16 */
    tFatType = FS_FAT16;
  }
  else
  {
    /* the rest is FAT32 */
    tFatType = FS_FAT32;
  }

  /* set values for fat12/16 and fat32 */
  ulMinFatClusters = 0;     /* set the default value */
  switch ( tFatType )
  {
    case FS_FAT12:
      /* max cluster value for a fat12 volume is 4084 */
//      ulMinFatClusters = 0;
      ulMaxFatClusters = 4084;
      uiFatElementSize = 12;
      /* reserved sectors for 12 is 1 */
      ulReservedSectors = 1;
      /* the root dir is not part of the fat area, it must be reserved as an extra area */
      ulRootDirEntries = 112;
      break;
    case FS_FAT16:
      /* max cluster value for a fat16 volume is 65524 */
      ulMinFatClusters = 4085;
      ulMaxFatClusters = 65524;
      uiFatElementSize = 16;
      /* reserved sectors for 16 is 1 */
      ulReservedSectors = 1;
      /* the root dir is not part of the fat area, it must be reserved as an extra area */
      ulRootDirEntries = 112;
      break;
    case FS_FAT32:
      /* max cluster value for a fat32 volume is 0x0fffffef */
      ulMaxFatClusters = 0x0ffffff0;
      uiFatElementSize = 32;
      /* reserved sectors for fat32 is 32 */
      ulReservedSectors = 32;
      /* root dir is in fat area, so no extra sectors are reserved */
      ulRootDirEntries = 0;
      break;
    default:
      /* strange fat typ */
      return (1 == 0);
  };

  /* get the number of sectors used for the root directory */
  ulRootDirSectors = (ulRootDirEntries * 32 + uiBytesPerSec - 1) / uiBytesPerSec;

  /* get the number of fat controlled sectors and the fat itself */
  ulFatControlledSectors = ulPartitionSectors -      /* all sectors in this partition */
                           ulReservedSectors;      /* number of reserved sectors */

  /* get the clustersize for the complete sectors */
  /* NOTE: this might be one sector too much, as the fat will not be counted in */
  ulClusterCnt = 1;
  ulSectorsPerCluster = 0;
  do
  {
    /* clustersize must be less or equal than 32k */
    ulClusterSize = ulClusterCnt * uiBytesPerSec;
    if ( ulClusterSize > 32768 )
    {
      /* the cluster grew too big, this volume can not be formatted */
      return (1 == 0);
    }
    /* get number of fat sectors */
    ulSectorCnt = 0;
    ulFatBitCount = 0;
    do
    {
        /* process one more sector */
        ++ulSectorCnt;
        /* this sector takes up one more fat element */
        ulFatBitCount += uiFatElementSize;
        /* round up the number of fat bits to the sector boundary */
        ulFatSizeSectors  = ulFatBitCount + 8 * uiBytesPerSec - 1;
        ulFatSizeSectors /= 8 * uiBytesPerSec;
    } while( (ulFatSizeSectors+ulSectorCnt)<ulFatControlledSectors );

    /* get number of clusters for this clustersize */
    ulNumberOfClusters  = ulFatControlledSectors;
    /* substract the fat sectors */
    ulNumberOfClusters -= ulFatSizeSectors;
    /* substract directory from fat controlled sectors for fat12 and fat16 */
    if( tFatType!=FS_FAT32 )
    {
        ulNumberOfClusters -= ulRootDirSectors;
    }
    /* round up to the current cluster size */
    ulNumberOfClusters += ulClusterCnt - 1;
    ulNumberOfClusters /= ulClusterCnt;
    /* number of clusters must be in the allowed range */
    if ( ulNumberOfClusters>=ulMinFatClusters && ulNumberOfClusters<=ulMaxFatClusters )
    {
      /* the number of clusters fits, accept this as the cluster size */
      ulSectorsPerCluster = ulClusterCnt;
      break;
    }
    /* try next bigger clustersize */
    ulClusterCnt <<= 1;
  }
  while (ulClusterCnt < 256 );

  /* check for a vaild number of clusters per sector */
  if ( ulSectorsPerCluster == 0 )
  {
    /* it's not valid */
    return (1 == 0);
  }

  /* guessed all values, fill the bootsector */

  /* clear the whole sector */
  memset(uBootSec.ab, 0, uiBytesPerSec);
  /* copy initial values for BS_jmpBoot and BS_OEMName */
  memcpy(&uBootSec.tFat, abBootSector0, 11);
  /* set bytes per sector */
  uBootSec.tFat.BPB_BytsPerSec = uiBytesPerSec;
  /* set sectors per cluster */
  uBootSec.tFat.BPB_SecPerClus = ulSectorsPerCluster;
  /* set number of reserved sectors */
  uBootSec.tFat.BPB_RsvdSecCnt = ulReservedSectors;
  /* set number of fats to 1 */
  uBootSec.tFat.BPB_NumFATs = 1;
  /* set number of entries in the root directory */
  uBootSec.tFat.BPB_RootEntCnt = ulRootDirEntries;
  /* set media type to 'fixed' */
  uBootSec.tFat.BPB_Media = 0xf8;
  /* ignore old stuff */
  uBootSec.tFat.BPB_SecPerTrk = 0;
  uBootSec.tFat.BPB_NumHeads = 0;
  /* no hidden sectors */
  uBootSec.tFat.BPB_HiddSec = 0;

  fatId.BS_DrvNum = 0;
  fatId.BS_Reserved1 = 0;
  fatId.BS_BootSig = 0x29;
  memcpy(fatId.BS_VolID, "abcd", 4);
  memcpy(fatId.BS_VolLab, "NO NAME    ", 11);
  switch (tFatType)
  {
    case FS_FAT12:
      memcpy(fatId.BS_FilSysType, "FAT12   ", 8);
      break;
    case FS_FAT16:
      memcpy(fatId.BS_FilSysType, "FAT16   ", 8);
      break;
    case FS_FAT32:
      memcpy(fatId.BS_FilSysType, "FAT     ", 8);
      break;
    default:
      break;
  };

  /* set fat type sepcific stuff */
  switch (tFatType)
  {
    case FS_FAT12:
    case FS_FAT16:
      uBootSec.tFat.BPB_TotSec16 = ulPartitionSectors;
      uBootSec.tFat.BPB_FATSz16 = ulFatSizeSectors;
      uBootSec.tFat.BPB_TotSec32 = 0;
      /* copy the id structure */
      memcpy(&uBootSec.tFat.BS_Spec1632.fat16, &fatId, sizeof(fat16_bootsec_t));
      break;

    case FS_FAT32:
      uBootSec.tFat.BPB_TotSec16 = 0;
      uBootSec.tFat.BPB_FATSz16 = 0;
      uBootSec.tFat.BPB_TotSec32 = ulPartitionSectors;
      uBootSec.tFat.BS_Spec1632.fat32.BPB_FATSz32 = ulFatSizeSectors;
      /* set only first fat active */
      uBootSec.tFat.BS_Spec1632.fat32.BPB_ExtFlags = 0x80;
      uBootSec.tFat.BS_Spec1632.fat32.BPB_FSVer = 0;
      /* set root directory cluster */
      uBootSec.tFat.BS_Spec1632.fat32.BPB_RootClus =
        uBootSec.tFat.BS_Spec1632.fat32.BPB_FSInfo = 1;
      uBootSec.tFat.BS_Spec1632.fat32.BPB_BkBootSec = 6;
      /* copy the id structure */
      memcpy(&uBootSec.tFat.BS_Spec1632.fat32.tFat16Part, &fatId, sizeof(fat16_bootsec_t));
      break;

    default:
      return (1 == 0);
  };

  /* set signature magic, even on non-standard sectors this is placed at offset 0x1FE */
  uBootSec.ab[510] = 0x55;
  uBootSec.ab[511] = 0xaa;

  /* write data to the first sector in the image */
  iResult = ptIo->fn_writeSectors(ptIo, 0, 1, uBootSec.ab);
  if ( iResult )
  {
    ulFirstRootDirSector = ulReservedSectors + ulFatSizeSectors;

    /* abuse the partition structure to pass all the values */
    tPartition.disc = ptIo;
    tPartition.filesysType = tFatType;
    tPartition.bytesPerCluster = uiBytesPerSec * ulSectorsPerCluster;
    tPartition.bytesPerSector = uiBytesPerSec;
    tPartition.numberOfSectors = ulPartitionSectors;
    tPartition.rootDirCluster = ulFirstRootDirSector / ulSectorsPerCluster;
    tPartition.rootDirStart = ulFirstRootDirSector % ulSectorsPerCluster;
    tPartition.sectorsPerCluster = ulSectorsPerCluster;
    tPartition.totalSize = ulPartitionSize;
    tPartition.fat.fatStart = ulReservedSectors;
    tPartition.fat.sectorsPerFat = ulFatSizeSectors;
    iResult = initFat(&tPartition, &uBootSec);
    if ( iResult )
    {
      iResult = initRootDir(&tPartition, &uBootSec, ulRootDirEntries);
    }
  }

  return iResult;
}

/*****************************************************************************/
/*! \}                                                                       */
/*****************************************************************************/
