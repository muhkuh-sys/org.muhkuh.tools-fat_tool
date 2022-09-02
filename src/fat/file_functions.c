
#include <string.h>
#include <stdio.h> // printf

#include "fat/common.h"
#include "fat/file_functions.h"
#include "fat/file_allocation_table.h"
#include "fat/partition.h"
#include "fat/bit_ops.h"
/*#include "main.h"*/
/*#include "md5sum.h"*/

const char *getFilenameExtension(const DIR_ENTRY *ptDirEntry)
{
    const char *pcLastDot;
    const char *pcSearch;
    const char *pcNameMaxEnd;
    char cFileNameChar;


    /* look for the last dot in the filename */

    /* no dot found yet */
    pcLastDot = NULL;

    /* start searching at the beginning of the filename */
    pcSearch = ptDirEntry->filename;
    pcNameMaxEnd = pcSearch + MAX_FILENAME_LENGTH - 1;
    while( pcSearch<pcNameMaxEnd && (cFileNameChar=*(pcSearch++)) )
    {
        if( cFileNameChar=='.' )
        {
            /* found a dot, get the character after the dot */
            pcLastDot = pcSearch;
        }
    }

    return pcLastDot;
}

int file_delete_direntry(PARTITION *ptPartition, DIR_ENTRY *ptDirEntry)
{
    unsigned long ulCluster;
    int iResult;
    DIR_ENTRY tDirContents;
    int iNextEntryFound;
    int iRet = 1;

    ulCluster = _FAT_directory_entryGetCluster(ptDirEntry->entryData);
    
    // If this is a directory, make sure it is empty
    iResult = _FAT_directory_isDirectory(ptDirEntry);
    if( iResult )
    {
        /* get the first directory entry */
        iNextEntryFound = _FAT_directory_getFirstEntry(ptPartition, &tDirContents, ulCluster);
        while (iNextEntryFound)
        {
            iResult = _FAT_directory_isDot(&tDirContents);
            if( !iResult )
            {
                // The directory had something in it that isn't a reference to itself or it's parent
                return 0;
            }
            iNextEntryFound = _FAT_directory_getNextEntry(ptPartition, &tDirContents);
        }
    }

    if(ulCluster != CLUSTER_FREE)
    {
        // Remove the cluster chain for this file
        iResult = _FAT_fat_clearLinks (ptPartition, ulCluster);
        if( !iResult )
        {
            iRet = 0;
        }
    }

    // Remove the directory entry for this file
    iResult = _FAT_directory_removeEntry (ptPartition, ptDirEntry);
    if( !iResult )
    {
        iRet = 0;
    }
    
    // Flush any sectors in the disc cache
    iResult = _FAT_cache_flush(ptPartition->cache);
    if( !iResult )
    {
        iRet = 0;
    }

    return iRet;
}


int FileCreate(PARTITION *ptPartition, const char *szFile, FILE_STRUCT *ptFile)
{
  DIR_ENTRY     tDirEntry;
  int           iResult;
  const char*   pcPathEnd;
  unsigned long ulDirCluster;


  // Search for the file on the disc
  iResult = _FAT_directory_entryFromPath(ptPartition, &tDirEntry, szFile, NULL);
  if ( iResult )
  {
    /* the file already exists */

    /* cowardly refuse to delete dirs */
    iResult = _FAT_directory_isDirectory(&tDirEntry);
    if ( iResult )
    {
      /* yes, it's a dir */
      return 0;
    }

    /* it's a file, delete it */
    iResult = file_delete_direntry(ptPartition, &tDirEntry);
    if ( !iResult )
    {
      /* failed to delete the file */
      return 0;
    }
  }

  /* now the file does not exist for sure, create a new one */

  /* get the directory from the path */ 
  pcPathEnd = strrchr (szFile, DIR_SEPARATOR);
  if (pcPathEnd == NULL)
  {
    /* the filename has no path elements */
    ulDirCluster = ptPartition->cwdCluster;
    pcPathEnd = szFile;
  }
  else
  {
    /* get the directory entry from the path part of the filename */
    iResult = _FAT_directory_entryFromPath(ptPartition, &tDirEntry, szFile, pcPathEnd);
    if( iResult )
    {
        /* the path part of the filename must point to a directory */
        iResult = _FAT_directory_isDirectory(&tDirEntry);
        if( iResult )
        {
          ulDirCluster = _FAT_directory_entryGetCluster (tDirEntry.entryData);
          /* Move the pathEnd past the last DIR_SEPARATOR */
          pcPathEnd += 1;
        }
        else
        {
            /* error: path points to a file */
            return 0;
        }
    }
    else
    {
        /* error: failed to resolve path */
        return 0;
    }
  }

  /* Create the entry data */
  strncpy(tDirEntry.filename, pcPathEnd, MAX_FILENAME_LENGTH - 1);
  memset(tDirEntry.entryData, 0, DIR_ENTRY_DATA_SIZE);

  iResult = _FAT_directory_addEntry(ptPartition, &tDirEntry, ulDirCluster);
  if(!iResult )
  {
    /* failed to add the direntry */
    return 0;
  } else
  {
    unsigned long ulFirstFileCluster;


    /* get free cluster for the file */
    ulFirstFileCluster = _FAT_fat_linkFreeCluster(ptPartition, CLUSTER_FREE);
    memset(ptFile, 0, sizeof(*ptFile));
    ptFile->ptPartition         = ptPartition;
    ptFile->ulStartCluster      = ulFirstFileCluster;
    ptFile->tDirEntryStart      = tDirEntry.dataStart;
    ptFile->tDirEntryEnd        = tDirEntry.dataEnd;
    ptFile->tPosition.ulCluster = ulFirstFileCluster;
  }
  return 1;
}


int FileClose(FILE_STRUCT* ptFile)
{
  u8 dirEntryData[DIR_ENTRY_DATA_SIZE];
  int iRet = 1;
  unsigned long ulOldFilesize;
  unsigned long ulOldStartCluster;
  
  // Load the old entry
  _FAT_cache_readPartialSector(ptFile->ptPartition->cache, 
                               dirEntryData, 
                               _FAT_fat_clusterToSector(ptFile->ptPartition, 
                                                        ptFile->tDirEntryEnd.cluster) + ptFile->tDirEntryEnd.sector,
                               ptFile->tDirEntryEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE,
                               ptFile->ptPartition->bytesPerSector);

  // Must the filesize and start cluster be updated?
  ulOldFilesize = u8array_to_u32(dirEntryData, DIR_ENTRY_fileSize);
  ulOldStartCluster = u8array_to_u16(dirEntryData, DIR_ENTRY_cluster) | (u8array_to_u16(dirEntryData, DIR_ENTRY_clusterHigh) << 16);
  if( ulOldFilesize!=ptFile->ulFilesize || ulOldStartCluster!=ptFile->ulStartCluster )
  {
    // Write new data to the directory entry
    // File size
    u32_to_u8array (dirEntryData, DIR_ENTRY_fileSize, ptFile->ulFilesize);
  
    // Start cluster
    u16_to_u8array (dirEntryData, DIR_ENTRY_cluster, ptFile->ulStartCluster);
    u16_to_u8array (dirEntryData, DIR_ENTRY_clusterHigh, ptFile->ulStartCluster >> 16);

    /* ATTENTION: Modification time and date */
//  u16_to_u8array (dirEntryData, DIR_ENTRY_mTime, _FAT_filetime_getTimeFromRTC());
//  u16_to_u8array (dirEntryData, DIR_ENTRY_mDate, _FAT_filetime_getDateFromRTC());
  
    /* ATTENTION: Modify Access date */
//  u16_to_u8array (dirEntryData, DIR_ENTRY_aDate, _FAT_filetime_getDateFromRTC());

    // Write the new entry
    _FAT_cache_writePartialSector(ptFile->ptPartition->cache, 
                                  dirEntryData, 
                                  _FAT_fat_clusterToSector(ptFile->ptPartition, ptFile->tDirEntryEnd.cluster) + ptFile->tDirEntryEnd.sector,
                                  ptFile->tDirEntryEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE,
                                  ptFile->ptPartition->bytesPerSector);
  }

  // Flush any sectors in the disc cache
  if (!_FAT_cache_flush(ptFile->ptPartition->cache)) 
  {
    iRet = 0;
  }
  
  return iRet;
}

int FileWrite(FILE_STRUCT* ptFile, const void* pvData, unsigned long ulDataLen)
{
  PARTITION*      ptPartition = ptFile->ptPartition;
  CACHE*          ptCache     = ptPartition->cache;  
  FILE_POSITION   tPosition;
  unsigned long   ulTempNextCluster; 
  int             iTempVar;
  unsigned long   ulRemain    = ulDataLen;
  int             fAppend     = 0;
  const unsigned char*  pbData      = (const unsigned char*)pvData;
  int             fNoError    = 1;
 

  tPosition = ptFile->tPosition;
  /* Check if we are appending */
  if( (ulDataLen + ptFile->ulCurrentPosition) > ptFile->ulFilesize) 
  {
    fAppend = 1;
  }
    
  // Move onto next cluster if needed
  if(tPosition.ulSector >= ptPartition->sectorsPerCluster) 
  {
    tPosition.ulSector = 0;
    ulTempNextCluster = _FAT_fat_nextCluster(ptPartition, tPosition.ulCluster);
    
    if( (ulTempNextCluster == CLUSTER_EOF) || (ulTempNextCluster == CLUSTER_FREE)) 
    {
      // Ran out of clusters so get a new one
      ulTempNextCluster = _FAT_fat_linkFreeCluster(ptPartition, tPosition.ulCluster);
    }
     
    if(ulTempNextCluster == CLUSTER_FREE) 
    {
      // Couldn't get a cluster, so abort
      return 0;
    } else 
    {
      tPosition.ulCluster = ulTempNextCluster;
    }
  }
  
  // Align to sector
  iTempVar = ptPartition->bytesPerSector - tPosition.ulByte;
  if((unsigned long)iTempVar > ulRemain) 
  {
    iTempVar = ulRemain;
  }

  if( (unsigned long)iTempVar < ptPartition->bytesPerSector) 
  {
    // Write partial sector to disk
    _FAT_cache_writePartialSector(ptCache, 
                                  pbData, 
                                  _FAT_fat_clusterToSector(ptPartition, tPosition.ulCluster) + tPosition.ulSector, 
                                  tPosition.ulByte, 
                                  iTempVar,
                                  ptFile->ptPartition->bytesPerSector);

    ulRemain         -= iTempVar;
    pbData           += iTempVar;
    tPosition.ulByte += iTempVar;
    
    // Move onto next sector
    if (tPosition.ulByte >= ptPartition->bytesPerSector) 
    {
      tPosition.ulByte = 0;
      ++tPosition.ulSector;
    }
  }

  // Align to cluster
  // tempVar is number of sectors to write
  if(ulRemain > (ptPartition->sectorsPerCluster - tPosition.ulSector) * ptPartition->bytesPerSector) 
  {
    iTempVar = ptPartition->sectorsPerCluster - tPosition.ulSector;
  } else 
  {
    iTempVar = ulRemain / ptPartition->bytesPerSector;
  }

  if(iTempVar > 0) 
  {
    _FAT_disc_writeSectors(ptPartition->disc, 
                           _FAT_fat_clusterToSector(ptPartition, 
                                                    tPosition.ulCluster) + tPosition.ulSector, 
                           iTempVar, 
                           pbData);
                           
    pbData   += iTempVar * ptPartition->bytesPerSector;
    ulRemain -= iTempVar * ptPartition->bytesPerSector;
    tPosition.ulSector += iTempVar;
  }

  if( (tPosition.ulSector >= ptPartition->sectorsPerCluster) && (ulRemain > 0)) 
  {
    tPosition.ulSector = 0;
    ulTempNextCluster = _FAT_fat_nextCluster(ptPartition, tPosition.ulCluster);
    
    if( (ulTempNextCluster == CLUSTER_EOF) || (ulTempNextCluster == CLUSTER_FREE)) {
      // Ran out of clusters so get a new one
      ulTempNextCluster = _FAT_fat_linkFreeCluster(ptPartition, tPosition.ulCluster);
    } 
    if(ulTempNextCluster == CLUSTER_FREE) 
    {
      // Couldn't get a cluster, so abort
      fNoError = 0;
    } else 
    {
      tPosition.ulCluster = ulTempNextCluster;
    }
  }

  // Write whole clusters
  while( (ulRemain >= ptPartition->bytesPerCluster) && fNoError) 
  {
    _FAT_disc_writeSectors(ptPartition->disc, 
                           _FAT_fat_clusterToSector(ptPartition, tPosition.ulCluster),
                           ptPartition->sectorsPerCluster, 
                           pbData);
    pbData   += ptPartition->bytesPerCluster;
    ulRemain -= ptPartition->bytesPerCluster;
    
    if(ulRemain > 0) 
    {
      ulTempNextCluster = _FAT_fat_nextCluster(ptPartition, tPosition.ulCluster);
      
      if( (ulTempNextCluster == CLUSTER_EOF) || (ulTempNextCluster == CLUSTER_FREE)) 
      {
        // Ran out of clusters so get a new one
        ulTempNextCluster = _FAT_fat_linkFreeCluster(ptPartition, tPosition.ulCluster);
      } 
      if (ulTempNextCluster == CLUSTER_FREE) 
      {
        // Couldn't get a cluster, so abort
        fNoError = 0;
      } else 
      {
        tPosition.ulCluster = ulTempNextCluster;
      }
    } else 
    {
      // Allocate a new cluster when next writing the file
      tPosition.ulSector = ptPartition->sectorsPerCluster;
    }
  }

  // Write remaining sectors
  iTempVar = ulRemain / ptPartition->bytesPerSector; // Number of sectors left
  
  if( (iTempVar > 0) && fNoError) 
  {
    _FAT_disc_writeSectors(ptPartition->disc, 
                           _FAT_fat_clusterToSector(ptPartition, tPosition.ulCluster), 
                           iTempVar,
                           pbData);
    pbData             += iTempVar * ptPartition->bytesPerSector;
    ulRemain           -= iTempVar * ptPartition->bytesPerSector;
    tPosition.ulSector += iTempVar;
  }
  
  // Last remaining sector
  if( (ulRemain > 0) && fNoError) 
  {
    if(fAppend) 
    {
      _FAT_cache_eraseWritePartialSector(ptCache, 
                                         pbData, 
                                         _FAT_fat_clusterToSector(ptPartition, tPosition.ulCluster) + tPosition.ulSector, 
                                         0, 
                                         ulRemain,
                                         ptFile->ptPartition->bytesPerSector);
    } else 
    {
      _FAT_cache_writePartialSector(ptCache, 
                                    pbData, 
                                    _FAT_fat_clusterToSector (ptPartition, tPosition.ulCluster) + tPosition.ulSector, 
                                    0, 
                                    ulRemain,
                                    ptFile->ptPartition->bytesPerSector);
    }
    tPosition.ulByte += ulRemain;
    ulRemain = 0;
  }

  // Amount read is the originally requested amount minus stuff remaining
  ulDataLen = ulDataLen - ulRemain;

  // Update file information
  ptFile->tPosition   = tPosition;
  ptFile->ulFilesize += ulDataLen;

  return ulDataLen;
}

int FileDelete(PARTITION *ptPartition, const char *szFile)
{
  int iResult;
  DIR_ENTRY tDirEntry;


  /* Search for the file on the disc */
  iResult = _FAT_directory_entryFromPath (ptPartition, &tDirEntry, szFile, NULL);
  if( iResult )
  {
    iResult = file_delete_direntry(ptPartition, &tDirEntry);
  }

  return iResult;
}


int FileExists(PARTITION *ptPartition, const char *szFile)
{
  DIR_ENTRY tDirEntry;


  // Search for the file on the disc
  return _FAT_directory_entryFromPath(ptPartition, &tDirEntry, szFile, NULL);
}


unsigned long GetFreeDiskSpace(const PARTITION *ptPartition)
{
  /* TODO: How to calculate free disk space */
/*
  u32 ulFatSector =  partition->fat.fatStart;

  while(ulFatSector < partition->fat.sectorsPerFat)
  {
    // Read FAT sector
    
    
    
    ++ulFatSector;
  } 
*/
  return 0xFFFFFFFF;  
}


int FileOpenForRead(PARTITION *ptPartition, const char *szFile, FILE_STRUCT *ptFile)
{
  DIR_ENTRY     tDirEntry;
  int           iResult;
  unsigned long ulFirstFileCluster;


  // Search for the file on the disc
  iResult = _FAT_directory_entryFromPath(ptPartition, &tDirEntry, szFile, NULL);
  if ( !iResult )
  {
    /* the file does not exists */
	return 0;
  }

  /* refuse to read dirs */
  iResult = _FAT_directory_isDirectory(&tDirEntry);
  if ( iResult )
  {
    /* yes, it's a dir */
    return 0;
  }

  ulFirstFileCluster = _FAT_directory_entryGetCluster(tDirEntry.entryData);
 
  memset(ptFile, 0, sizeof(*ptFile));
  ptFile->ptPartition         = ptPartition;
  ptFile->ulFilesize          = u8array_to_u32(tDirEntry.entryData, DIR_ENTRY_fileSize);
  ptFile->ulStartCluster      = ulFirstFileCluster;
  ptFile->tDirEntryStart      = tDirEntry.dataStart;
  ptFile->tDirEntryEnd        = tDirEntry.dataEnd;
  ptFile->tPosition.ulCluster = ulFirstFileCluster;

  return 1;
}

static int file_inc_position(PARTITION *ptPartition, FILE_POSITION *ptPosition, unsigned long ulChunk)
{
    unsigned long ulNextCluster;
    unsigned long ulBlockSize;
    int iResult;


    /* assume success */
    iResult = 1;

    /* get the blocksize */
    ulBlockSize = ptPartition->disc->ulBlockSize;

    /* increase byte counter */
    ptPosition->ulByte += ulChunk;

    /* crossed sector boundary? */
    if( ptPosition->ulByte >= ulBlockSize )
    {
        /* yes, move to next sector */
        ptPosition->ulByte -= ulBlockSize;
        ++ptPosition->ulSector;

        /* is at least one sector left in the current cluster? */
        if( ptPosition->ulSector>=ptPartition->sectorsPerCluster )
        {
            /* no more sectors left in the current cluster -> move to the next cluster */
            ulNextCluster = _FAT_fat_nextCluster(ptPartition, ptPosition->ulCluster);
            if( (ulNextCluster==CLUSTER_EOF) || (ulNextCluster==CLUSTER_FREE) )
            {
                /* reached the file end */
                iResult = 0;
            }
            else
            {
                ptPosition->ulSector = 0;
                ptPosition->ulCluster = ulNextCluster;
            }
        }
    }

    /* return the errorflag */
    return iResult;
}


int FileRead(FILE_STRUCT *ptFile, void *pvData, unsigned long ulDataLen)
{
    PARTITION *ptPartition;
    CACHE *ptCache;
    unsigned long ulRemain;
    FILE_POSITION tPosition;
    unsigned long ulChunk;
    unsigned long ulBlockSize;
    unsigned char *pbData;
    unsigned long ulSector;
    int iResult;


    /* cast data pointer to something with a size */
    pbData = (unsigned char*)pvData;

    /* get the partition and cache */
    ptPartition = ptFile->ptPartition;
    ptCache = ptPartition->cache;

    /* get the block size */
    ulBlockSize = ptPartition->disc->ulBlockSize;

    /* Don't try to read if the read pointer is past the end of file */
    if( ptFile->ulCurrentPosition>=ptFile->ulFilesize )
    {
        return 0;
    }
    
    /* limit the read operation to the remaining file size */
    if( ulDataLen + ptFile->ulCurrentPosition > ptFile->ulFilesize )
    {
        ulDataLen = ptFile->ulFilesize - ptFile->ulCurrentPosition;
    }

    /* set the number of bytes left in this read operation */
    ulRemain = ulDataLen;

    tPosition = ptFile->tPosition;

    /* get the number of bytes to read in the first sector */
    ulChunk = ulBlockSize - tPosition.ulByte;
    /* limit read chunk to the requested size and the remaining sector length */
    if( ulChunk>ulRemain )
    {
        ulChunk = ulRemain;
    }

    /* assume success */
    iResult = 1;

    /* read a sector part? */
    if( ulChunk<ulBlockSize ) 
    {
        /* yes, the read request starts somewhere inside a sector */
        ulSector = _FAT_fat_clusterToSector(ptPartition, tPosition.ulCluster) + tPosition.ulSector;
        _FAT_cache_readPartialSector(ptCache, pbData, ulSector, tPosition.ulByte, ulChunk, ulBlockSize);
            
        ulRemain -= ulChunk;
        pbData += ulChunk;

        /* increase position */
        iResult = file_inc_position(ptPartition, &tPosition, ulChunk);
    }

    /* read complete sectors */
    while( iResult!=0 && ulRemain>=ulBlockSize )
    {
        /* read one complete sector */
        ulSector = _FAT_fat_clusterToSector(ptPartition, tPosition.ulCluster) + tPosition.ulSector;
        _FAT_disc_readSectors(ptPartition->disc, ulSector, 1, pbData);
        /* one sector processed */
        pbData += ulBlockSize;
        ulRemain -= ulBlockSize;
        /* increase position */
        iResult = file_inc_position(ptPartition, &tPosition, ulBlockSize);

        /* file_inc_position will try to switch to next cluster if possible
           and returns an error on EOF. */
        if( (0 == iResult)  &&
            (0 == ulRemain) )
        {
          unsigned long ulNewFilePosition = ptFile->ulCurrentPosition + ulDataLen;
          
          if(ulNewFilePosition == ptFile->ulFilesize)
          {
            /* We will override this error here, as we have already read the whole file */
            iResult = 1;
          }
        }
    }

    /* read partial sector? */
    if( iResult!=0 && ulRemain>0 )
    {
        /* read part of the sector */
        ulSector = _FAT_fat_clusterToSector(ptPartition, tPosition.ulCluster) + tPosition.ulSector;
        _FAT_cache_readPartialSector(ptCache, pbData, ulSector, 0, ulRemain, ulBlockSize);
        /* increase position */
        iResult = file_inc_position(ptPartition, &tPosition, ulRemain);        
        /* all requested data was read */
        ulRemain = 0;
    }

    if( iResult )
    {
        /* all operations ok -> the return value is the number of processed bytes */
        iResult = ulDataLen-ulRemain;
    }
    else
    {
        /* return -1 for all errors */
        iResult = -1;
    }

    /* write the position info back */
    ptFile->tPosition = tPosition;
    ptFile->ulCurrentPosition += ulDataLen-ulRemain;

    /* return the error flag */
    return iResult;
}

int FileMakeDir(PARTITION* ptPartition, const char *path) 
{
  bool        fFileExists;
  DIR_ENTRY   dirEntry;
  const char* pathEnd;
  u32         parentCluster, dirCluster;
  u8          newEntryData[DIR_ENTRY_DATA_SIZE];
  
  // Search for the file/directory on the disc
  fFileExists = _FAT_directory_entryFromPath(ptPartition, &dirEntry, path, NULL);
  
  // Make sure it doesn't exist
  if (fFileExists) 
  {
    printf("File exists\n");
    return 0;
  }
  
  // Get the directory it has to go in 
  pathEnd = strrchr (path, DIR_SEPARATOR);
  if (pathEnd == NULL) 
  {
    // No path was specified
    parentCluster = ptPartition->cwdCluster;
    pathEnd = path;
  } else 
  {
    // Path was specified -- get the right parentCluster
    // Recycling dirEntry, since it needs to be recreated anyway
    if( !_FAT_directory_entryFromPath(ptPartition, &dirEntry, path, pathEnd) ||
        !_FAT_directory_isDirectory(&dirEntry)) 
    {
      printf("not a directory/entry not found\n");
      return 0;
    }
    parentCluster = _FAT_directory_entryGetCluster (dirEntry.entryData);
    // Move the pathEnd past the last DIR_SEPARATOR
    pathEnd += 1;
  }
  
  // Create the entry data
  strncpy (dirEntry.filename, pathEnd, MAX_FILENAME_LENGTH - 1);
  memset (dirEntry.entryData, 0, DIR_ENTRY_DATA_SIZE);
  
  // Set the creation time and date
  dirEntry.entryData[DIR_ENTRY_cTime_ms] = 0;
  
  // Set the directory attribute
  dirEntry.entryData[DIR_ENTRY_attributes] = ATTRIB_DIR;
  
  // Get a cluster for the new directory
  dirCluster = _FAT_fat_linkFreeClusterCleared (ptPartition, CLUSTER_FREE);
  if (dirCluster == CLUSTER_FREE) 
  {
    // No space left on disc for the cluster
    printf("No space left on disc for the cluster\n");
    return 0;
  }
  u16_to_u8array (dirEntry.entryData, DIR_ENTRY_cluster, dirCluster);
  u16_to_u8array (dirEntry.entryData, DIR_ENTRY_clusterHigh, dirCluster >> 16);

  // Write the new directory's entry to it's parent
  if (!_FAT_directory_addEntry(ptPartition, &dirEntry, parentCluster)) 
  {
    printf("_FAT_directory_addEntry failed\n");
    return 0;
  }
  
  // Create the dot entry within the directory
  memset (newEntryData, 0, DIR_ENTRY_DATA_SIZE);
  memset (newEntryData, ' ', 11);
  newEntryData[DIR_ENTRY_name] = '.';
  newEntryData[DIR_ENTRY_attributes] = ATTRIB_DIR;
  u16_to_u8array (newEntryData, DIR_ENTRY_cluster, dirCluster);
  u16_to_u8array (newEntryData, DIR_ENTRY_clusterHigh, dirCluster >> 16);
  
  // Write it to the directory, erasing that sector in the process
  _FAT_cache_eraseWritePartialSector(ptPartition->cache, 
                                     newEntryData, 
                                     _FAT_fat_clusterToSector(ptPartition, dirCluster), 
                                     0, 
                                     DIR_ENTRY_DATA_SIZE,
                                     ptPartition->bytesPerSector);
  
  // Create the double dot entry within the directory
  newEntryData[DIR_ENTRY_name + 1] = '.';
  u16_to_u8array (newEntryData, DIR_ENTRY_cluster, parentCluster);
  u16_to_u8array (newEntryData, DIR_ENTRY_clusterHigh, parentCluster >> 16);

  // Write it to the directory
  _FAT_cache_writePartialSector(ptPartition->cache, newEntryData, 
                                _FAT_fat_clusterToSector(ptPartition, dirCluster), 
                                DIR_ENTRY_DATA_SIZE, 
                                DIR_ENTRY_DATA_SIZE,
                                ptPartition->bytesPerSector);

  // Flush any sectors in the disc cache
  if(!_FAT_cache_flush(ptPartition->cache)) 
  {
    printf("_FAT_cache_flush failed \n");
    return 0;
  }

  return 1;
}
#if 0
#define __MD5_BUFFER_SIZE__ 64
static unsigned char abMd5Buffer[__MD5_BUFFER_SIZE__];
 
int FileCalculateMD5(char* szFile, unsigned char *abMD5)
{
    md5_state_t tMd5State;
    FILE_STRUCT tFile;
    unsigned long ulFileLeft;
    unsigned long uiChunkSize;
    int iResult;

    /* init the md5 struct */
    md5_init(&tMd5State);

    /* open the file for reading */
    iResult = FileOpenForRead(g_ptDefaultPartition, szFile, &tFile);
    if( !iResult )
    {
        /* failed to open the file for reading */
        return 0;
    }


    /* loop over the complete file */
    ulFileLeft = tFile.ulFilesize;
    while( ulFileLeft!=0 )
    {
        /* get the segment size, limit it to the buffer */
        uiChunkSize = ulFileLeft;
        if( uiChunkSize>__MD5_BUFFER_SIZE__ )
        {
            uiChunkSize = __MD5_BUFFER_SIZE__;
        }
        /* read data from file */
        iResult = FileRead(&tFile, abMd5Buffer, uiChunkSize);
        if( !iResult )
        {
            /* failed to read data */
            return 0;
        }
        /* add new data to md5 sum */
        md5_append(&tMd5State, abMd5Buffer, uiChunkSize);
        /* dec filesize */
        ulFileLeft -= uiChunkSize;
    }

    /* finalize md5 sum and pass the result */
    md5_finish(&tMd5State, abMD5);

    /* all ok! */
    return 1;
}
#endif

unsigned long FileReadClusterchain(FILE_STRUCT *ptFile, CLUSTER_CHAIN *ptClusterChain, unsigned long ulMaxTableEntries)
{
    PARTITION *ptPartition;
    CACHE *ptCache;
    unsigned long ulBytesPerCluster;
    unsigned long ulClusterCnt;
    unsigned long ulStartCluster;
    unsigned long ulNextCluster;
    unsigned long ulCurrentCluster;
    unsigned long ulChunkSize;
    unsigned long ulSectorSize;


    /* get the partition and cache */
    ptPartition = ptFile->ptPartition;
    ptCache = ptPartition->cache;

    /* init cluster counter */
    ulClusterCnt = 0;

    ulSectorSize = ptPartition->bytesPerSector;

    /* get the current cluster */
    ulStartCluster = ptFile->ulStartCluster;

    ulBytesPerCluster = ptPartition->bytesPerCluster;

    /* get the sector list */
    do
    {
        /* try to group the clusters */
        ulNextCluster = ulStartCluster;
        ulChunkSize = 0;
        do
        {
            /* increase the chunk size */
            ulChunkSize += ulBytesPerCluster;
            /* move to the new cluster */
            ulCurrentCluster = ulNextCluster;
            /* get the next cluster */
            ulNextCluster = _FAT_fat_nextCluster(ptPartition, ulCurrentCluster);
        } while( ulNextCluster==ulCurrentCluster+1 );

        /* write the new element */
        ptClusterChain->ulSector = _FAT_fat_clusterToSector(ptPartition, ulStartCluster) * ulSectorSize + ptPartition->disc->ulStartOffset;
        ptClusterChain->ulSize = ulChunkSize/sizeof(uint32_t);
        ++ptClusterChain;
        ++ulClusterCnt;
        if( ulClusterCnt>ulMaxTableEntries )
        {
            /* error, too many clusters */
            ulClusterCnt = 0;
            break;
        }

        /* move to the new cluster */
        ulStartCluster = ulNextCluster;
    } while( ulStartCluster!=CLUSTER_EOF );

    /* return the number of entries in the table */
    return ulClusterCnt;
}

