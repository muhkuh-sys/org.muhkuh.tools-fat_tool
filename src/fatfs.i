%module fatfs

%{
	#include "fatfs.h"
typedef struct
{
	char *pcData;
	size_t sizData;
} tBinaryData;

typedef struct
{
	char *pcData;
	size_t sizData;
} tBinaryDataFree;
%}

%typemap(in, numinputs = 1) (const char *pcData, size_t sizData)
{
	$1 = (char*)lua_tolstring(L, $argnum, &$2);
}

%typemap(out) tBinaryData
{
	if ($1.pcData != NULL) {
		lua_pushlstring(L, $1.pcData, $1.sizData);
		++SWIG_arg;
	}
}

%typemap(out) tBinaryDataFree
{
	if ($1.pcData != NULL) {
		lua_pushlstring(L, $1.pcData, $1.sizData);
		++SWIG_arg;
		free($1.pcData);
	}
}

%feature("compactdefaultargs") fatfs::create;
%feature("compactdefaultargs") fatfs::mount;
%feature("compactdefaultargs") fatfs::dir;
%feature("compactdefaultargs") fatfs::cd;
%feature("compactdefaultargs") fatfs::writefile;
class fatfs
{
public:
	bool mkdir(char* pszPath);
	bool cd(char* pszPath = "\\");
	bool dir(char* pszPath = ".", bool fRecursive = false);
	bool writefile(const char *pcData, size_t sizData, char* pszPath);
	bool writeraw(const char *pcData, size_t sizData, size_t sizOffset);
	bool deletefile(char* pszPath);
	bool fileexists(char* pszPath);	
};

%extend fatfs {
	void __gc(){
		delete self;
	}

	static fatfs* create(size_t sizSectorSize, size_t sizNumSectors, size_t sizTotalSize = 0, size_t sizOffset = 0){
		fatfs* fs = new fatfs(sizSectorSize, sizNumSectors, sizTotalSize, sizOffset);
		
		if (fs->checkReady()) {
			return fs;
		} else {
			delete fs;
			return NULL;
		}
	}

	
	static fatfs* mount(const char *pcData, size_t sizData, size_t sizOffset = 0){
		fatfs* fs = new fatfs(pcData, sizData, sizOffset);
		
		if (fs->checkReady()) {
			return fs;
		} else {
			delete fs;
			return NULL;
		}
	}
	
	tBinaryDataFree readfile(char* pszPath){
		tBinaryDataFree tData;
		tData.pcData = self->readfile(pszPath, &tData.sizData);
		return tData;
	}
	
	tBinaryData readraw(size_t sizOffset, size_t sizLen) {
		tBinaryData tData;
		tData.pcData = self->readraw(sizOffset, sizLen);
		tData.sizData = sizLen;
		return tData;
	}
	
	tBinaryData getimage(){
		tBinaryData tData;	
		tData.pcData = self->getimage((unsigned long*) &tData.sizData);
		return tData;
	}
};
