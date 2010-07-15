#ifndef __FATFS_H__
#define __FATFS_H__


#include "fat/partition.h"
#include "fat/disk_io.h"


class fatfs
{
public:
	fatfs();
	/* 
	   allocates totalSize bytes,
	   formats a FAT filesystem at offset spanning numSectors sectors,
	   mounts the filesystem
	 */

	fatfs(size_t sizSectorSize, size_t sizNumSectors, size_t sizTotalSize, size_t sizOffset);

	/*
		Assumes that totalSize is sectorSize * numSectors and offset is 0
	*/
	//fatfs(size_t sizSectorSize, size_t sizNumSectors);

	/*
		Mounts a filessystem in a flash image.
		Makes a copy of the image.
	*/
	fatfs(const char* pabData, size_t sizDataLen, size_t sizSectorSize, size_t sizOffset, size_t sizNumSectors);

	/*
		Mounts a filessystem in a flash image.
		Makes a copy of the image.
	*/
	fatfs::fatfs(const char* pabData, size_t sizDataLen, size_t sizOffset);

	/*
		Check if m_PACKED_PST is true. If not, print a warning.
		Returns the value of PACKED_PST.
	*/
	bool checkReady();

	/*
		Shuts down the partition and frees the image
	*/
	void destroy(void);

	/*
		Shuts down the partition and frees the image
	*/
	~fatfs();

	/*
		Creates a file with the given name/path containing the given data
		returns true if successful
	*/
	bool writefile(const char* pabData, size_t sizFileLen, char* pszPath);

	/*
		If the file exists, the contents are read into a newly-allocated buffer and
		its address and size are returned. Otherwise, returns NULL.
	*/
	char* readfile(char* pszPath, size_t *psizLen);

	/*
		Delets a file at the given path
		returns true if the file could be deleted, false if it does not exist or if an error occurred.
	*/
	bool deletefile(char* pszPath);


	/*
		Checks if a file exists.
		returns true if the file exists, false if it does not exist or if an error occurred.
	*/
	bool fileexists(char* pszPath);

    /*
		Creates a directory at the given path
		returns true if successful
	*/
	bool mkdir(char* pszPath);

	/*
		Sets path as the current directory.
		Returns true if successful, false otherwise.
	*/
	bool cd(char* pszPath);

	/*
		prints a directory listing
	*/
	bool dir(char* pszPath, bool fRecursive);

	/*
		Returns the whole image
	*/
	char* getimage(unsigned long *pulSize);

	/*
		Writes raw data into the image (for 2nd stage loader)
		returns true if successful
	*/
	bool writeraw(const char* pabData, size_t sizFileLen, size_t sizOffset);

	/* 
		Reads raw data from the image
	*/
	char* readraw(size_t sizOffset, size_t sizLen);


private:
	bool					m_fReady;
	PARTITION*              m_ptRamDiskPartition;
	IO_INTERFACE			m_tIoIfRamdisk;
	void*					m_pvDiskMem;
	size_t					m_sizDiskMemSize;
};


#endif  // __MHASH_STATE_H__

