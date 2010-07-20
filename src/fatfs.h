#ifndef __FATFS_H__
#define __FATFS_H__

#include "fat/partition.h"
#include "fat/disk_io.h"
#include "fat/directory.h"

#include <stdio.h>
#include <stdarg.h>
#define MESSAGE(strFormat, ...) m_pfnvprintf(m_pvUser, strFormat, __VA_ARGS__);
//#define ERRORMESSAGE(strFormat, ...) m_pfnErrorHandler(m_pvUser, strFormat, __VA_ARGS__);
#define FAILHARD(strFormat, ...) m_pfnErrorHandler(m_pvUser, strFormat, __VA_ARGS__);
#define FAILSOFT(strFormat, ...) m_pfnvprintf(m_pvUser, strFormat, __VA_ARGS__);

typedef void (*FN_FATFS_ERROR_HANDLER)(void *pvUser, const char* strFormat, ...);
typedef void (*FN_FATFS_VPRINTF)(void *pvUser, const char* strFormat, ...);

class fatfs
{
public:
	fatfs();

	/* set error and print handlers (for Lua binding */
	void setHandlers(FN_FATFS_ERROR_HANDLER pfnErrorHandler, FN_FATFS_VPRINTF pfn_vprintf, void* pvUser);


	/* copy the current error handlers into the disc IO structure */
	void setDiscIOErrorHandlers();


	/* 
	   allocates totalSize bytes,
	   formats a FAT filesystem at offset spanning numSectors sectors,
	   mounts the filesystem
	 */
	bool create(size_t sizSectorSize, size_t sizNumSectors, size_t sizTotalSize, size_t sizOffset);

	//fatfs(size_t sizSectorSize, size_t sizNumSectors, size_t sizTotalSize, size_t sizOffset);

	/*
		Mounts a filessystem in a flash image.
		Makes a copy of the image.
	*/
	bool mount(const char* pabData, size_t sizDataLen, size_t sizOffset);

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


	enum Filetypes {TYPE_NONE, TYPE_FILE, TYPE_DIRECTORY};

	/*
		returns the type of entry under pszPath (file, directory or none if the entry does not exist)
	*/
	Filetypes gettype(char* pszPath);


	/*
		Checks if a file or directory exists under the given path.
	*/
	bool fileexists(char* pszPath);

	/*
		Returns true if the entry exists and is a directory.
		Returns false if it is a file or does not exist.
	*/
	bool isdir(char* pszPath);

	/*
		Returns true if the entry exists and is a file.
		Returns false if it is a directory or does not exist.
	*/
	bool isfile(char* pszPath);

	/* 
		Get file size. Returns -1 if an error occurred.
	*/
	long getfilesize(char* pszPath);


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
		prints the directory starting at a given cluster
		pszPath is only printed
	*/
	bool dir(char* pszPath, u32 dircluster, bool fRecursive);

	/*
		Find first cluster of the directory at path.
		in: pszPath
		out: pulClusterNo
		returns true if pulClusterNo could be set, false otherwise.
	*/
	bool get_dir_start_cluster(char* pszPath, u32 *pulClusterNo);

	/*
		gets first entry of the directory at dir cluster
		in: ulDirCluster
		out: ptDirEntry
		returns true if dir entry is valid
	*/
	bool getfirstdirentry(DIR_ENTRY *ptDirEntry, unsigned long ulDirCluster);

	/*
		gets next directory entry.
		in/out ptDirEntry
		returns true if ptDirEntry is a new directory entry
	*/
	bool getnextdirentry(DIR_ENTRY *ptDirEntry);

	/* 
		Gets the size of the file pointed to by ptDirEntry.
		in: ptDirEntry
	*/
	unsigned long getfilesize(DIR_ENTRY *ptDirEntry);

private:
	bool					m_fReady;
	PARTITION*              m_ptRamDiskPartition;
	IO_INTERFACE			m_tIoIfRamdisk;
	void*					m_pvDiskMem;
	size_t					m_sizDiskMemSize;

	FN_FATFS_ERROR_HANDLER  m_pfnErrorHandler;
	FN_FATFS_VPRINTF        m_pfnvprintf;
	void*                   m_pvUser;
	static void error(void *pvUser, const char* strFmt, ...);
	static void printMessage(void *pvUser, const char* strFmt, ...);


};


#endif  // __MHASH_STATE_H__

