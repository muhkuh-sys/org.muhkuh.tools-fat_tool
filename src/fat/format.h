#ifndef FORMAT_H_
#define FORMAT_H_

#include "fat/common.h"
#include "fat/disk_io.h"

#pragma pack(push, 1)
/*****************************************************************************/
/*! fat 12/16 specific part of bootsector                                    */
/*****************************************************************************/
PACKED_PRE
typedef struct
{
  uint8_t  BS_DrvNum;        /*!< irq 0x13 drive number  */
  uint8_t  BS_Reserved1;     /*!< reserved for nt crap   */
  uint8_t  BS_BootSig;       /*!< extended boot signature (0x29) */
  uint8_t  BS_VolID[4];      /*!< volume id  */
  char     BS_VolLab[11];    /*!< volume label */
  char     BS_FilSysType[8]; /*!< file system type ("FAT12   ", "FAT16   " or "FAT     ") */
} PACKED_PST fat16_bootsec_t;

/*****************************************************************************/
/*! fat 32 specific part of bootsector                                       */
/*****************************************************************************/
PACKED_PRE
typedef struct
{
  uint32_t  BPB_FATSz32;      /*!< fat32 count of sectors for one fat */
  uint16_t  BPB_ExtFlags;     /*!< mirroring flags */
  uint16_t  BPB_FSVer;        /*!< fat32 version number */
  uint32_t  BPB_RootClus;     /*!< first cluster of the root directory */
  uint16_t  BPB_FSInfo;       /*!< sector number of the fsinfo structure */
  uint16_t  BPB_BkBootSec;    /*!< sector number of a boot record copy */
  uint8_t   BPB_Reserved[12]; /*!< blah */
  fat16_bootsec_t tFat16Part;
} PACKED_PST fat32_bootsec_t;

/*****************************************************************************/
/*! fat bootsector                                                           */
/*****************************************************************************/
PACKED_PRE
typedef struct
{
  uint8_t  BS_jmpBoot[3];    /*!< jump  */
  char           BS_OEMName[8];    /*!< blah  */
  uint16_t BPB_BytsPerSec;   /*!< bytes per sector */
  uint8_t  BPB_SecPerClus;   /*!< sectors per cluster */
  uint16_t BPB_RsvdSecCnt;   /*!< number of reserved sectors */
  uint8_t  BPB_NumFATs;      /*!< number of fat structures */
  uint16_t BPB_RootEntCnt;   /*!< number of directory entries */
  uint16_t BPB_TotSec16;     /*!< 16 bit total count of sectors */
  uint8_t  BPB_Media;        /*!< media type */
  uint16_t BPB_FATSz16;      /*!< fat12/16 count of sectors for one fat, 0 for fat32 */
  uint16_t BPB_SecPerTrk;    /*!< sectors per track for irq 0x13 */
  uint16_t BPB_NumHeads;     /*!< number of heads for irq 0x13 */
  uint32_t BPB_HiddSec;      /*!< number of hidden sectors */
  uint32_t BPB_TotSec32;     /*!< 32 bit total count of sectors */
  PACKED_PRE
	union                            /* fat 12/16/32 specific data */
  {
    fat16_bootsec_t fat16;
    fat32_bootsec_t fat32;
  } PACKED_PST BS_Spec1632;
}
PACKED_PST fat_bootsec_t;

/*-----------------------------------*/

/*****************************************************************************/
/*! FAT directory entry                                                      */
/*****************************************************************************/
PACKED_PRE
typedef struct
{
  char            name[11];     /*!< 8+3 name,
                                     name[0]==0xe5 -> free entry,
                                     name[0]==0x00 -> entry free and end of dir
                                     name[0]==0x05 -> free entry, kanji save (0xe5 is a valid kanji code) */
  uint8_t   attr;         /*!< attributes */
  uint8_t   ntRes;        /*!< reserved for nt crap */
  uint8_t   crtTimeTenth; /*!< milisecond creation time stamp */
  uint16_t  crtTime;      /*!< creation time stamp */
  uint16_t  crtDate;      /*!< creation date stamp */
  uint16_t  lstAccTime;   /*!< last access time */
  uint16_t  fstClusHi;    /*!< high word of the first cluster number (always 0 for FAT12/16) */
  uint16_t  wrtTime;      /*!< time of last write */
  uint16_t  wrtDate;      /*!< date of last write */
  uint16_t  fstClusLo;    /*!< low word of the first cluster number */
  uint32_t   fileSize;     /*!< file size in bytes */
}
PACKED_PST fat_direntry_t;
#pragma pack(pop)


#define FatDirAttr_ReadOnly   0x01
#define FatDirAttr_Hidden     0x02
#define FatDirAttr_System     0x04
#define FatDirAttr_VolumeID   0x08
#define FatDirAttr_Directory  0x10
#define FatDirAttr_Archive    0x20
#define FatDirAttr_LongName   0x0f

/*-----------------------------------*/

unsigned long CalculateSectorSize(IO_INTERFACE *ptIo);
int formatFat(IO_INTERFACE *ptIo);

/*-----------------------------------*/

#endif /*FORMAT_H_*/
