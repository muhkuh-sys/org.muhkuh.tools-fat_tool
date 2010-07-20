#include "fatfs.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "fat/bit_ops.h"
#include "fat/common.h"
#include "fat/cache.h"
#include "fat/directory.h"
#include "fat/file_allocation_table.h"
#include "fat/format.h"
#include "fat/file_functions.h"
#include "ramdisk/interface.h"

/*
	bool					m_fReady;
	PARTITION*              m_ptRamDiskPartition;
	IO_INTERFACE			m_tIoIfRamdisk;
	void*					m_pvDiskMem;
	size_t					m_sizDiskMemSize;
*/


fatfs::fatfs(){
	//MESSAGE("Default constructor");
	m_fReady = false;
	m_ptRamDiskPartition = NULL;
	m_pvDiskMem = NULL;
	m_sizDiskMemSize = 0;
	m_pfnErrorHandler = &fatfs::error;
	m_pfnvprintf = &fatfs::printMessage;
	m_pvUser = NULL;
}

void fatfs::setHandlers(FN_FATFS_ERROR_HANDLER pfnErrorHandler, FN_FATFS_VPRINTF pfn_vprintf, void* pvUser){
	m_pfnErrorHandler = pfnErrorHandler;
	m_pfnvprintf = pfn_vprintf;
	m_pvUser = pvUser;
}

void fatfs::error(void *pvUser, const char* strFmt, ...){
	va_list argp;	
	va_start(argp, strFmt);
	vprintf(strFmt, argp);
	printf("\n");
	va_end(argp);
}

void fatfs::printMessage(void *pvUser, const char* strFmt, ...){
	va_list argp;	
	va_start(argp, strFmt);
	vprintf(strFmt, argp);
	printf("\n");
	va_end(argp);
}

bool fatfs::create(size_t sizSectorSize, size_t sizNumSectors, size_t sizTotalSize, size_t sizOffset){
	int iResult;
	if (sizTotalSize == 0) sizTotalSize = sizSectorSize * sizNumSectors;

	if (sizOffset > sizTotalSize  ||
		sizSectorSize * sizNumSectors > sizTotalSize||
		sizOffset + sizSectorSize * sizNumSectors > sizTotalSize ) {
		FAILHARD("fatfs create: Illegal size/offset parameters");
		return false;
	}

	m_pvDiskMem = malloc(sizTotalSize);
	if (m_pvDiskMem == NULL){
		FAILHARD("fatfs create: Could not allocate memory for image");
		return false;
	}
	//MESSAGE("malloc 0x%08p", m_pvDiskMem);
	m_sizDiskMemSize = sizTotalSize;

	// init memory to $ff - should this be a parameter?
	memset(m_pvDiskMem, 0xff, m_sizDiskMemSize);

	/* set the ramdisk IO interface */
	m_tIoIfRamdisk = g_tIoIfRamDisk;
	m_tIoIfRamdisk.ulBlockSize        = (unsigned long) sizSectorSize;
	m_tIoIfRamdisk.pvUser             = (void*)((char*)m_pvDiskMem + sizOffset);
	m_tIoIfRamdisk.ulStartOffset      = 0;
	m_tIoIfRamdisk.ulDiskSize         = (unsigned long) (sizSectorSize * sizNumSectors);
	
	_FAT_disc_startup(&m_tIoIfRamdisk); // does nothing
	/* format and mount file system */
	iResult = formatFat(&m_tIoIfRamdisk); 
	if (iResult==0){
		free (m_pvDiskMem); m_pvDiskMem = NULL;
		FAILHARD("fatfs create: formatFat failed");
		return false;
	}

	/* format successful, try to mount the new image */
	m_ptRamDiskPartition = _FAT_partition_mountCustomInterface(&m_tIoIfRamdisk, 0);
	if (m_ptRamDiskPartition == NULL){
		free (m_pvDiskMem); m_pvDiskMem = NULL;
		FAILHARD("fatfs create: Could not mount partition");
		return false;
	} else {
		MESSAGE("Partition created. %d sectors  %d bytes/sector  offset: 0x%x  image size: 0x%x",
			sizNumSectors, sizSectorSize, sizOffset, sizTotalSize);
		m_fReady = true;
		return true;
	}
}



bool fatfs::mount(const char* pabData, size_t sizTotalSize, size_t sizOffset){
	size_t sizSectorSize;
	size_t sizNumSectors;

	if (sizOffset > sizTotalSize){
		FAILHARD("fatfs mount: Illegal offset>size");
		return false;
	}

	//bool _FAT_partition_recognize ( void* pvImage, size_t sizImage, size_t sizPartitionOffset, 
	//								size_t *psizBytesPerSector, size_t *psizNumberOfSectors);
	if (!_FAT_partition_recognize((void*) pabData, sizTotalSize, sizOffset, &sizSectorSize, &sizNumSectors)) {
		FAILSOFT("fatfs mount: FAT boot sector not found or invalid");
		return false;
	}

	m_pvDiskMem = malloc(sizTotalSize);
	if (m_pvDiskMem == NULL){
		FAILHARD("fatfs mount: Could not allocate memory for image");
		return false;
	}
	m_sizDiskMemSize = sizTotalSize;
	memcpy(m_pvDiskMem, pabData, sizTotalSize);

	/* set the ramdisk IO interface */
	m_tIoIfRamdisk = g_tIoIfRamDisk;
	m_tIoIfRamdisk.ulBlockSize        = (unsigned long) sizSectorSize;
	m_tIoIfRamdisk.pvUser             = (void*)((char*)m_pvDiskMem + sizOffset);
	m_tIoIfRamdisk.ulStartOffset      = 0;
	m_tIoIfRamdisk.ulDiskSize         = (unsigned long) (sizSectorSize * sizNumSectors);
	
	_FAT_disc_startup(&m_tIoIfRamdisk); // does nothing

	/* try to mount the new image */
	m_ptRamDiskPartition = _FAT_partition_mountCustomInterface(&m_tIoIfRamdisk, 0);
	if (m_ptRamDiskPartition == NULL){
		free (m_pvDiskMem); m_pvDiskMem = NULL;
		FAILSOFT("fatfs mount: Could not mount partition");
		return false;
	} else {
		MESSAGE("Partition mounted. %d sectors  %d bytes/sector  offset: 0x%x  image size: 0x%x",
			sizNumSectors, sizSectorSize, sizOffset, sizTotalSize);
		m_fReady = true;
		return true;
	}
}

void fatfs::destroy(void){
	m_fReady = false;
	if (m_ptRamDiskPartition!= NULL) {
		bool fOk = _FAT_partition_unmount(m_ptRamDiskPartition);
		if (!fOk) {
			FAILHARD("fatfs dtor: Partition could not be properly unmounted, there are open files!"); //Todo: message?
			_FAT_partition_unsafeUnmount(m_ptRamDiskPartition);
		}
		m_ptRamDiskPartition = NULL;
		//MESSAGE("partition unmounted");
	}

	if (m_pvDiskMem!=NULL) {
		//MESSAGE("free 0x%08p", m_pvDiskMem);
		free(m_pvDiskMem);
		m_pvDiskMem = NULL;
		//MESSAGE("disk buffer freed");
	}
}

fatfs::~fatfs(void){
	//MESSAGE("~fatfs");
	destroy();
}

bool fatfs::checkReady() {
	if (m_fReady==false) {
		FAILHARD("fatfs instance is not ready");
	}
	return m_fReady;
}	


char* fatfs::getimage(unsigned long *pulSize){
	if (pulSize != NULL) *pulSize = (unsigned long) m_sizDiskMemSize;
	return (char*) m_pvDiskMem;
}

bool fatfs::writeraw(const char* pabData, size_t sizFileLen, size_t sizOffset){
	if (!checkReady()) return false;
	if (pabData==NULL) {
		FAILHARD("writeraw: data is nil");		
		return false;
	}

	if (sizOffset + sizFileLen > m_sizDiskMemSize) {
		FAILHARD("writeraw: offset/length exceed disk size");
		return false;
	}

	memcpy((void*) ((char*)m_pvDiskMem + sizOffset), pabData, sizFileLen);
	MESSAGE("writeraw: wrote %d bytes at offset %d", sizFileLen, sizOffset);
	return true;
}

char* fatfs::readraw(size_t sizOffset, size_t sizLen){
	if (!checkReady()) return NULL;
	if (sizOffset + sizLen > m_sizDiskMemSize) {
		FAILHARD("readraw: offset/length exceed disk size");
		return NULL;
	}
	
	MESSAGE("readraw: read %d bytes at offset %d", sizLen, sizOffset);
	return ((char*)m_pvDiskMem) + sizOffset;
}


bool fatfs::mkdir(char* pszPath){
	if (!checkReady()) return false;
	int iResult = FileMakeDir(m_ptRamDiskPartition, pszPath);
	if (iResult==1){
		MESSAGE("Created directory %s", pszPath);
		return true;
	} else {
		FAILHARD("Could not create directory %s", pszPath);
		return false;
	}	
}

bool fatfs::cd(char* pszPath){
	bool fOk;
	if (!checkReady()) return false;
	fOk = _FAT_directory_chdir(m_ptRamDiskPartition, pszPath);
	if (!fOk) {
		FAILHARD("Could not chdir to %s", pszPath); //todo: Message?
	}
	return fOk;
}

bool fatfs::writefile(const char *pcData, size_t sizData, char* pszPath){
	FILE_STRUCT tFile;	
	int iResult;

	if (!checkReady()) return false;
	iResult = FileCreate(m_ptRamDiskPartition, pszPath, &tFile);
	if (iResult==0) {
		FAILHARD("writefile %s: FileCreate failed", pszPath);
		return false;
	}

	size_t sizBytesWritten = FileWrite(&tFile, pcData, (unsigned long) sizData);
	if (sizBytesWritten != sizData) {
		FileClose(&tFile);
		deletefile(pszPath);
		FAILHARD("writefile %s: FileWrite failed", pszPath);
		return false;
	}

	iResult = FileClose(&tFile);
	if (iResult==0) {
		FAILHARD("writefile %s: FileClose failed", pszPath);
		return false;
	} else {
		MESSAGE("File %s written", pszPath);
		return true;
	}
}



// int FileOpenForRead(PARTITION *ptPartition, const char *szFile, FILE_STRUCT *ptFile)
// int FileRead(FILE_STRUCT* ptFile, void* pvData, unsigned long ulDataLen);
char* fatfs::readfile(char* pszPath, size_t *psizLen){
	FILE_STRUCT tFile;
	int iResult;
	void *pabData;
	unsigned long ulLen;

	if (!checkReady()) return NULL;
	iResult = FileOpenForRead(m_ptRamDiskPartition, pszPath, &tFile);
	if (iResult == 0){
		FAILHARD("readfile %s: FileOpenForRead failed ", pszPath); // todo: Message?
		return NULL;
	}

	ulLen = tFile.ulFilesize;
	pabData = malloc(tFile.ulFilesize);
	if (pabData == NULL){
		FileClose(&tFile);
		FAILHARD("readfile %s: Could not allocate memory for file contents", pszPath);
		return NULL;
	}

	iResult = FileRead(&tFile, pabData, tFile.ulFilesize);
	if (iResult != tFile.ulFilesize) {
		FAILHARD("readfile %s: FileRead returned an error", pszPath);
		free(pabData);
		FileClose(&tFile);
		return NULL;
	}

	FileClose(&tFile);
	*psizLen = tFile.ulFilesize;

	MESSAGE("File %s read", pszPath);
	return (char*)pabData;
}

// int FileDelete(PARTITION *ptPartition, const char *szFile);
bool fatfs::deletefile(char* pszPath){
	int iResult;
	if (!checkReady()) return false;
	iResult = FileDelete(m_ptRamDiskPartition, pszPath);

	if (iResult==0) {
		FAILHARD("Could not delete file %s", pszPath); // Todo: Message?
		return false;
	} else {
		MESSAGE("File %s deleted", pszPath);
		return true;
	}

}

// int FileExists(PARTITION *ptPartition, const char *szFile);
bool fatfs::fileexists(char* pszPath){
	int iResult;
	if (!checkReady()) return false;
	iResult = FileExists(m_ptRamDiskPartition, pszPath);
	return iResult==1;
}

fatfs::Filetypes fatfs::gettype(char* pszPath) {
	DIR_ENTRY     tDirEntry;
	int           iResult;
	if (!checkReady()) return TYPE_NONE;

	iResult = _FAT_directory_entryFromPath(m_ptRamDiskPartition, &tDirEntry, pszPath, NULL);
	if ( !iResult ){
		return TYPE_NONE;
	} else if (_FAT_directory_isDirectory(&tDirEntry)) {
		return TYPE_DIRECTORY;
	} else {
		return TYPE_FILE;
	}
}


bool fatfs::isdir(char* pszPath) {
	return TYPE_DIRECTORY==gettype(pszPath);
}

bool fatfs::isfile(char* pszPath) {
	return TYPE_FILE==gettype(pszPath);
}


long fatfs::getfilesize(char* pszPath) {
	DIR_ENTRY tDirEntry;

	if (!checkReady()) return -1;
	if (!_FAT_directory_entryFromPath(m_ptRamDiskPartition, &tDirEntry, pszPath, NULL)){
		MESSAGE("getfilesize %s: not found ", pszPath);
		return -1;
	}
	if (_FAT_directory_isDirectory(&tDirEntry)) {
		MESSAGE("getfilesize %s: is a directory ", pszPath);
		return -1;
	}
	return (long) getfilesize(&tDirEntry);
}


unsigned long fatfs::getfilesize(DIR_ENTRY *ptDirEntry) {
	return u8array_to_u32(ptDirEntry->entryData, DIR_ENTRY_fileSize);
}


bool fatfs::get_dir_start_cluster(char* pszPath, u32 *pulClusterNo){
	DIR_ENTRY tDirEntry;

	if (_FAT_directory_entryFromPath(m_ptRamDiskPartition, &tDirEntry, pszPath, NULL) &&
		_FAT_directory_isDirectory(&tDirEntry)) {
		*pulClusterNo =  _FAT_directory_entryGetCluster (tDirEntry.entryData);;
		return true;
	} else {
		MESSAGE("directory %s does not exist", pszPath);
		return false;
	}
}

bool fatfs::getfirstdirentry(DIR_ENTRY *ptDirEntry, unsigned long ulDirCluster) {
	return _FAT_directory_getFirstEntry (m_ptRamDiskPartition, ptDirEntry, ulDirCluster);
}

bool fatfs::getnextdirentry(DIR_ENTRY *ptDirEntry) {
	return _FAT_directory_getNextEntry (m_ptRamDiskPartition, ptDirEntry);
}


bool fatfs::dir(char* pszPath, u32 dircluster, bool fRecursive){
	DIR_ENTRY tDirEntry;

	MESSAGE("Directory '%s'", pszPath);

	/* 1 list subdirs */
	if (!getfirstdirentry(&tDirEntry, dircluster)){
		FAILHARD("_FAT_directory_getFirstEntry failed");
		return false;
	}
	do {
		if (_FAT_directory_isDirectory(&tDirEntry)) {
			MESSAGE("  DIR         %-15s", tDirEntry.filename);
		}
	} while (getnextdirentry(&tDirEntry));

	/* 2 list the files */
	if (!getfirstdirentry(&tDirEntry, dircluster)){
		FAILHARD("_FAT_directory_getFirstEntry failed");
		return false;
	}
	do {
		if (!_FAT_directory_isDirectory(&tDirEntry)) {
			MESSAGE("  %-10d  %-15s", getfilesize(&tDirEntry), tDirEntry.filename);
		}
	} while (getnextdirentry(&tDirEntry));

	/* 3 recurse into subdirs */
	if (fRecursive) {
		if (!getfirstdirentry(&tDirEntry, dircluster)){
			FAILHARD("_FAT_directory_getFirstEntry failed");
			return false;
		}
		do {
			if (_FAT_directory_isDirectory(&tDirEntry) && !_FAT_directory_isDot(&tDirEntry)) {
				if (!dir(tDirEntry.filename, 
						_FAT_directory_entryGetCluster (tDirEntry.entryData),
						fRecursive)) 
				{
					return false;
				}
			}
		} while (getnextdirentry(&tDirEntry));
	}
	return true;
}

bool fatfs::dir(char* pszPath, bool fRecursive){
	u32 ulDirCluster;
	if (!checkReady()) return false;
	if (pszPath == NULL) {
		return false;
	}
	if (get_dir_start_cluster(pszPath, &ulDirCluster)){
		return dir(pszPath, ulDirCluster, fRecursive);
	} else {
		return false;
	}
}
