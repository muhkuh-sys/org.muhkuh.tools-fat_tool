#ifndef FILE_FUNCTIONS_H_
#define FILE_FUNCTIONS_H_


#include "fat/partition.h"
#include "fat/directory.h"

typedef struct FILE_POSITIONtag
{
  unsigned long ulCluster;
  unsigned long ulSector;
  unsigned long ulByte;
  
} FILE_POSITION;

typedef struct {
  PARTITION*         ptPartition;
  unsigned long      ulFilesize;
  unsigned long      ulStartCluster;
  unsigned long      ulCurrentPosition;
  FILE_POSITION      tPosition;
  DIR_ENTRY_POSITION tDirEntryStart;   // Points to the start of the LFN entries of a file, or the alias for no LFN
  DIR_ENTRY_POSITION tDirEntryEnd;     // Always points to the file's alias entry
} FILE_STRUCT;

typedef struct {
  unsigned long ulSector;
  unsigned long ulSize;
} CLUSTER_CHAIN;

int FileCreate(PARTITION *ptPartition, const char *szFile, FILE_STRUCT *ptFile);
int FileExists(PARTITION *ptPartition, const char *szFile);
int FileClose(FILE_STRUCT* ptFile);
int FileWrite(FILE_STRUCT* ptFile, const void* pvData, unsigned long ulDataLen);
int FileDelete(PARTITION *ptPartition, const char *szFile);
int FileOpenForRead(PARTITION *ptPartition, const char *szFile, FILE_STRUCT *ptFile);
int FileRead(FILE_STRUCT* ptFile, void* pvData, unsigned long ulDataLen);
int FileMakeDir(PARTITION* ptPartition, const char *path); 
//int FileCalculateMD5(char* szFile, unsigned char *abMD5);
unsigned long FileReadClusterchain(FILE_STRUCT *ptFile, CLUSTER_CHAIN *ptClusterChain, unsigned long ulMaxTableEntries);

int file_delete_direntry(PARTITION *ptPartition, DIR_ENTRY *ptDirEntry);
const char *getFilenameExtension(const DIR_ENTRY *ptDirEntry);

unsigned long GetFreeDiskSpace(const PARTITION *ptPartition);

#endif /*FILE_FUNCTIONS_H_*/
