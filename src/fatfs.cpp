#include "fatfs.h"


#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "fat/common.h"
#include "fat/cache.h"
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

// define USE_WXLOG to use the dll under wxLua (Muhkuh), undefine it to use the dll under normal Lua
//#define USE_WXLOG
#ifdef USE_WXLOG
	#include <wx/wx.h>
	#define MESSAGE(strFormat, ...) wxLogMessage(wxT(strFormat), __VA_ARGS__)
#else
	#define MESSAGE(strFormat, ...) printf(strFormat "\n", __VA_ARGS__)
#endif
#define ERRORMESSAGE MESSAGE

fatfs::fatfs(){
	MESSAGE("Default constructor");
	m_fReady = false;
	m_ptRamDiskPartition = NULL;
	m_pvDiskMem = NULL;
	m_sizDiskMemSize = 0;
}

fatfs::fatfs(size_t sizSectorSize, size_t sizNumSectors, size_t sizTotalSize, size_t sizOffset){
	int iResult;
	m_fReady = false;
	m_ptRamDiskPartition = NULL;
	m_pvDiskMem = NULL;
	m_sizDiskMemSize = 0;

	if (sizTotalSize == 0) sizTotalSize = sizSectorSize * sizNumSectors;

	if (sizOffset > sizTotalSize  ||
		sizSectorSize * sizNumSectors > sizTotalSize||
		sizOffset + sizSectorSize * sizNumSectors > sizTotalSize ) {
		ERRORMESSAGE("fatfs create: Illegal size/offset parameters");
		return;
	}

	m_pvDiskMem = malloc(sizTotalSize);
	if (m_pvDiskMem == NULL){
		ERRORMESSAGE("fatfs create: Could not allocate memory for image");
		return;
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
		ERRORMESSAGE("fatfs create: formatFat failed");
	}

	/* format successful, try to mount the new image */
	m_ptRamDiskPartition = _FAT_partition_mountCustomInterface(&m_tIoIfRamdisk, 0);
	if (m_ptRamDiskPartition == NULL){
		ERRORMESSAGE("fatfs create: Could not mount partition");
	} else {
		MESSAGE("Partition created. %d sectors  %d bytes/sector  offset: 0x%x  image size: 0x%x",
			sizNumSectors, sizSectorSize, sizOffset, sizTotalSize);
		m_fReady = true;
	}

}

// not used
fatfs::fatfs(const char* pabData, size_t sizDataLen, size_t sizSectorSize, size_t sizOffset, size_t sizNumSectors){
	m_fReady = false;
	m_ptRamDiskPartition = NULL;
	m_pvDiskMem = NULL;
	m_sizDiskMemSize = 0;
	m_pvDiskMem = malloc(sizDataLen);
	if (m_pvDiskMem == NULL){
		ERRORMESSAGE("fatfs ctor: Could not allocate memory for image");
		return;
	}

	if (sizNumSectors==0) {
		sizNumSectors = sizDataLen/sizSectorSize;
		if (sizNumSectors * sizSectorSize != sizDataLen) {
			ERRORMESSAGE("fatfs ctor: Image size is not a multiple of sector size");
			return;
		}
	}

	if (sizDataLen < sizOffset ||
		sizDataLen < sizSectorSize * sizNumSectors ||
		sizSectorSize * sizNumSectors < sizOffset ) {
		ERRORMESSAGE("fatfs ctor: Illegal size/offset parameters");
		return;
	}

	m_sizDiskMemSize = sizDataLen;

	memcpy(m_pvDiskMem, pabData, sizDataLen);

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
		ERRORMESSAGE("fatfs ctor: Could not mount partition");
	} else {
		MESSAGE("Partition mounted. %d sectors  %d bytes/sector  offset: 0x%x  image size: 0x%x",
			sizNumSectors, sizSectorSize, sizOffset, sizDataLen);
		m_fReady = true;
	}
}

fatfs::fatfs(const char* pabData, size_t sizDataLen, size_t sizOffset){
	size_t sizSectorSize;
	size_t sizNumSectors;

	m_fReady = false;
	m_ptRamDiskPartition = NULL;
	m_sizDiskMemSize = 0;
	m_pvDiskMem = NULL;

	//bool _FAT_partition_recognize ( void* pvImage, size_t sizImage, size_t sizPartitionOffset, 
	//								size_t *psizBytesPerSector, size_t *psizNumberOfSectors);
	if (!_FAT_partition_recognize((void*) pabData, sizDataLen, sizOffset, &sizSectorSize, &sizNumSectors)) {
		ERRORMESSAGE("fatfs mount: FAT boot sector not found or invalid");
		return;
	}

	m_pvDiskMem = malloc(sizDataLen);
	if (m_pvDiskMem == NULL){
		ERRORMESSAGE("fatfs mount: Could not allocate memory for image");
		return;
	}
	m_sizDiskMemSize = sizDataLen;
	memcpy(m_pvDiskMem, pabData, sizDataLen);

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
		ERRORMESSAGE("fatfs mount: Could not mount partition");
	} else {
		MESSAGE("Partition mounted. %d sectors  %d bytes/sector  offset: 0x%x  image size: 0x%x",
			sizNumSectors, sizSectorSize, sizOffset, sizDataLen);
		m_fReady = true;
	}
}

void fatfs::destroy(void){
	m_fReady = false;
	if (m_ptRamDiskPartition!= NULL) {
		bool fOk = _FAT_partition_unmount(m_ptRamDiskPartition);
		if (!fOk) {
			ERRORMESSAGE("fatfs dtor: Partition could not be properly unmounted, there are open files!");
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
		ERRORMESSAGE("fatfs instance is not ready");
	}
	return m_fReady;
}	


char* fatfs::getimage(unsigned long *pulSize){
	if (pulSize != NULL) *pulSize = (unsigned long) m_sizDiskMemSize;
	return (char*) m_pvDiskMem;
}

bool fatfs::writeraw(const char* pabData, size_t sizFileLen, size_t sizOffset){
	if (!checkReady()) return false;
	if (pabData==NULL) return false;
	if (sizOffset + sizFileLen > m_sizDiskMemSize) {
		ERRORMESSAGE("writeraw: offset/length exceed disk size");
		return false;
	}

	memcpy((void*) ((char*)m_pvDiskMem + sizOffset), pabData, sizFileLen);
	MESSAGE("writeraw: wrote %d bytes at offset %d", sizFileLen, sizOffset);
	return true;
}

char* fatfs::readraw(size_t sizOffset, size_t sizLen){
	if (!checkReady()) return NULL;
	if (sizOffset + sizLen > m_sizDiskMemSize) {
		ERRORMESSAGE("readraw: offset/length exceed disk size");
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
		ERRORMESSAGE("Could not create directory %s", pszPath);
		return false;
	}	
}

bool fatfs::cd(char* pszPath){
	bool fOk;
	if (!checkReady()) return false;
	fOk = _FAT_directory_chdir(m_ptRamDiskPartition, pszPath);
	if (!fOk) {
		ERRORMESSAGE("Could not chdir to directory %s", pszPath);
	}
	return fOk;
}

// bool _FAT_directory_entryFromPath (PARTITION* partition, DIR_ENTRY* entry, const char* path, const char* pathEnd);

bool fatfs::dir(char* pszPath, bool fRecursive){
	bool fOk;
	int iResult;
	u32 oldcwd, cwd;
	DIR_ENTRY tDirEntry;
	FILE_STRUCT tFile;
	if (!checkReady()) return false;

	if (pszPath != NULL) {

		MESSAGE("Directory '%s'", pszPath);
		oldcwd = m_ptRamDiskPartition->cwdCluster;

		if (pszPath!=NULL) {
			fOk = _FAT_directory_chdir(m_ptRamDiskPartition, pszPath);
			if (!fOk) {
				ERRORMESSAGE("Path not found");
				return false;
			}
		}

		cwd = m_ptRamDiskPartition->cwdCluster;

		/* 1 list subdirs */
		fOk = _FAT_directory_getFirstEntry (m_ptRamDiskPartition, 
	                                                &tDirEntry, 
		                                            cwd);
		if (!fOk) {
			ERRORMESSAGE("_FAT_directory_getFirstEntry failed");
			return false;
		}

		do {
			if (_FAT_directory_isDirectory(&tDirEntry)) {
				MESSAGE("  DIR         %-15s", tDirEntry.filename);
			}         //123456789012
		} while (_FAT_directory_getNextEntry (m_ptRamDiskPartition, &tDirEntry));

		/* 2 list the files */
		fOk = _FAT_directory_getFirstEntry (m_ptRamDiskPartition, 
	                                                &tDirEntry, 
		                                            cwd);
		if (!fOk) {
			ERRORMESSAGE("_FAT_directory_getFirstEntry failed");
			return false;
		}

		do {
			if (!_FAT_directory_isDirectory(&tDirEntry)) {
	
				int FileOpenForRead(PARTITION *ptPartition, const char *szFile, FILE_STRUCT *ptFile);
				iResult = FileOpenForRead(m_ptRamDiskPartition, tDirEntry.filename, &tFile);
				if (iResult == 1){
					MESSAGE("  %-10d  %-15s", tFile.ulFilesize, tDirEntry.filename);
					FileClose(&tFile);
				} else {
					MESSAGE("              %-15s Failed to open", tDirEntry.filename);
				}
			}
		} while (_FAT_directory_getNextEntry (m_ptRamDiskPartition, &tDirEntry));

		/* 3 recurse into subdirs */
		if (fRecursive) {

			fOk = _FAT_directory_getFirstEntry (m_ptRamDiskPartition, 
	                                                &tDirEntry, 
		                                            cwd);

			if (!fOk) {
			ERRORMESSAGE("_FAT_directory_getFirstEntry failed");
				return false;
			}

			do {
				if (_FAT_directory_isDirectory(&tDirEntry) && !_FAT_directory_isDot(&tDirEntry)) {
					dir(tDirEntry.filename, fRecursive);
				}
			} while (_FAT_directory_getNextEntry (m_ptRamDiskPartition, &tDirEntry));

		}
		m_ptRamDiskPartition->cwdCluster = oldcwd;
		
	}
	return true;
}

bool fatfs::writefile(const char *pcData, size_t sizData, char* pszPath){
	FILE_STRUCT tFile;	
	int iResult;

	if (!checkReady()) return false;
	iResult = FileCreate(m_ptRamDiskPartition, pszPath, &tFile);

	if (iResult==0) {
		ERRORMESSAGE("writefile %s: FileCreate failed", pszPath);
		return false;
	}

	size_t sizBytesWritten = FileWrite(&tFile, pcData, (unsigned long) sizData);
	if (sizBytesWritten != sizData) {
		ERRORMESSAGE("writefile %s: FileWrite failed", pszPath);
		FileClose(&tFile);
		deletefile(pszPath);
		return false;
	}

	iResult = FileClose(&tFile);
	if (iResult==0) {
		ERRORMESSAGE("writefile %s: FileClose failed", pszPath);
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
		ERRORMESSAGE("readfile %s: FileOpenForRead failed ", pszPath);
		return NULL;
	}

	ulLen = tFile.ulFilesize;
	pabData = malloc(tFile.ulFilesize);
	if (pabData == NULL){
		ERRORMESSAGE("readfile %s: Could not allocate memory for file contents", pszPath);
		FileClose(&tFile);
		return NULL;
	}

	iResult = FileRead(&tFile, pabData, tFile.ulFilesize);
	if (iResult != tFile.ulFilesize) {
		ERRORMESSAGE("readfile %s: FileRead returned an error", pszPath);
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
		ERRORMESSAGE("Could not delete file %s", pszPath);
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




