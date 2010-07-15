#ifndef DISK_IO_H_
#define DISK_IO_H_

#define FEATURE_MEDIUM_CANREAD    0x00000001
#define FEATURE_MEDIUM_CANWRITE   0x00000002

#define IO_TYPE_SPI				1
#define IO_TYPE_RAM				2
#define IO_TYPE_PARFLASH	3
#define IO_TYPE_SDMMC     4

struct IO_INTERFACE_STRUCT;

typedef int (* FN_MEDIUM_STARTUP)(const struct IO_INTERFACE_STRUCT* ptData);
typedef int (* FN_MEDIUM_ISINSERTED)(const struct IO_INTERFACE_STRUCT* ptData);
typedef int (* FN_MEDIUM_READSECTORS)(const struct IO_INTERFACE_STRUCT* ptData, unsigned long sector, unsigned long numSectors, void* buffer);
typedef int (* FN_MEDIUM_WRITESECTORS)(const struct IO_INTERFACE_STRUCT* ptData, unsigned long sector, unsigned long numSectors, const void* buffer);
typedef int (* FN_MEDIUM_CLEARSTATUS)(const struct IO_INTERFACE_STRUCT* ptData);
typedef int (* FN_MEDIUM_SHUTDOWN)(const struct IO_INTERFACE_STRUCT* ptData);

struct IO_INTERFACE_STRUCT {
  unsigned long           ioType ;
  unsigned long           features ;
  FN_MEDIUM_STARTUP       fn_startup ;
  FN_MEDIUM_ISINSERTED    fn_isInserted ;
  FN_MEDIUM_READSECTORS   fn_readSectors ;
  FN_MEDIUM_WRITESECTORS  fn_writeSectors ;
  FN_MEDIUM_CLEARSTATUS   fn_clearStatus ;
  FN_MEDIUM_SHUTDOWN      fn_shutdown ;
  unsigned long           ulBlockSize;
  unsigned long           ulPagePerSecCnt;
  void                   *pvUser;
  unsigned long           ulStartOffset;
  unsigned long           ulDiskSize;
} ;

typedef struct IO_INTERFACE_STRUCT IO_INTERFACE ;


#endif /*DISK_IO_H_*/
