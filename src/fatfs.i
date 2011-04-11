%module fatfs

%{
	#include "fatfs.h"
	#include "fat/bit_ops.h"
	#include "fat/directory.h"
	
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


/***************************************************************************
	Error Handler
	Format the error message and push the formatted string on the Lua stack
	signal an error
***************************************************************************/
void fatfs_error_handler(void* pvUser, const char* strFormat, ...) {
	va_list argp;	
	char buf[1024];
	lua_State* L; 
	if (pvUser) {
		L = (lua_State*)pvUser; 
		lua_getglobal(L, "print");  
		if (lua_iscfunction(L, -1)) {
			va_start(argp, strFormat);
			_vsnprintf(buf, 1023, strFormat, argp);
			va_end(argp);
			lua_pushstring(L, buf);
			lua_error(L);
		} else {
			lua_pop(L, 1);
		}
	}	
}


/***************************************************************************
	Print handler
	Get the Lua print function. If available, 
	format the message and push the formatted string on the Lua stack,
	then call the print function
***************************************************************************/
void fatfs_snprintf(void* pvUser, const char* strFormat, ...) {
	va_list argp;	
	char buf[1024];
	lua_State* L; 
	if (pvUser) {
		L = (lua_State*)pvUser; 
		lua_getglobal(L, "print");  
		if (lua_iscfunction(L, -1)) {
			va_start(argp, strFormat);
			_vsnprintf(buf, 1023, strFormat, argp);
			va_end(argp);
			lua_pushstring(L, buf);
			lua_call(L, 1, 0);  
		} else {
			lua_pop(L, 1);
		}
	}	
}

%}

/****************************************************************************
	Convert an input parameter string
	Use $input instead of $argnum. $argnum leads to wrong index in mount function 
	$input            - Input object holding value to be converted.	
****************************************************************************/
%typemap(in, numinputs = 1) (const char *pcData, size_t sizData)
%{
	$1 = (char*)lua_tolstring(L, $input, &$2);
	if ($1==NULL) SWIG_fail_arg("$symname", $argnum,"char *");
%}

/***************************************************************************
	Return a memory block as a Lua string.
	Creates a copy in Lua.
***************************************************************************/
%typemap(out) tBinaryData
{
	if ($1.pcData != NULL) {
		lua_pushlstring(L, $1.pcData, $1.sizData);
		++SWIG_arg;
	}
}

/***************************************************************************
	Return a memory block as a string.
	Creates copy in Lua and frees the original memory block.
***************************************************************************/
%typemap(out) tBinaryDataFree
{
	if ($1.pcData != NULL) {
		lua_pushlstring(L, $1.pcData, $1.sizData);
		++SWIG_arg;
		free($1.pcData);
	}
}

/***************************************************************************
	Return a number
***************************************************************************/
%typemap(out) long
%{
	if ($1>=0) {
		lua_pushnumber(L, $1);
		++SWIG_arg;		
	}
%}


/****************************************************************************
	This typemap passes the Lua state to the function. This allows the function
	to call functions of the Swig Runtime API and the Lua C API.
****************************************************************************/
 
%typemap(in, numinputs = 0) lua_State *
%{
	$1 = L;
%}


%typemap(in, numinputs = 0) int *piNumResults
%{
	int i;
	$1 = &i;
%}

%typemap(argout, numinputs = 0) int *piNumResults
%{
	SWIG_arg+=*$1;
%}



%feature("compactdefaultargs") create;
%feature("compactdefaultargs") mount;
%feature("compactdefaultargs") fatfs::dir;
%feature("compactdefaultargs") fatfs::cd;
%feature("compactdefaultargs") fatfs::writefile;
class fatfs
{
public:
	bool mkdir(char* pszPath);
	bool cd(char* pszPath = "/" );//"\\");
	bool dir(char* pszPath = ".", bool fRecursive = false);
	bool writefile(const char *pcData, size_t sizData, char* pszPath);
	bool writeraw(const char *pcData, size_t sizData, size_t sizOffset);
	bool deletefile(char* pszPath);
	bool fileexists(char* pszPath);	
	long getfilesize(char* pszPath);	
	enum Filetypes {TYPE_NONE, TYPE_FILE, TYPE_DIRECTORY};
	Filetypes gettype(char* pszPath);
	bool isfile(char* pszPath);
	bool isdir(char* pszPath);
};

%extend fatfs {
	void __gc(){
		delete self;
	}

	static fatfs* create(lua_State *L, size_t sizSectorSize, size_t sizNumSectors, size_t sizTotalSize = 0, size_t sizOffset = 0){
		fatfs* fs = new fatfs();
		fs->setHandlers(fatfs_error_handler, fatfs_snprintf, L);
		if (fs->create(sizSectorSize, sizNumSectors, sizTotalSize, sizOffset)){
			return fs;
		} else {
			delete fs;
			return NULL;
		}
	}

	static fatfs* mount(lua_State *L, const char *pcData, size_t sizData, size_t sizOffset = 0 ){
		fatfs* fs = new fatfs();
		fs->setHandlers(fatfs_error_handler, fatfs_snprintf, L);
		if (fs->mount(pcData, sizData, sizOffset)) {
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


	/* 
		L: in
		piNumResults: out
		pszPath: in
	*/
	void getdirentries(lua_State *L, int *piNumResults, char* pszPath){
		u32 ulDirCluster;
		DIR_ENTRY tDirEntry;
		bool fIsDir;
		unsigned long ulFilesize;
		int iEntryCount;
		
		*piNumResults = 0;			
		if (!self->checkReady()) return;	
		if (!self->get_dir_start_cluster(pszPath, &ulDirCluster)) return;

		lua_newtable(L);
		*piNumResults = 1;	
		iEntryCount = 0;
		
		if( self->getfirstdirentry(&tDirEntry, ulDirCluster)) do {
			fIsDir = _FAT_directory_isDirectory(&tDirEntry);
			
			lua_newtable(L);
			lua_pushstring(L, tDirEntry.filename);
			lua_setfield(L, -2, "name");
			if (!fIsDir) {
				ulFilesize = self->getfilesize(&tDirEntry);
				lua_pushnumber(L, ulFilesize);
				lua_setfield(L, -2, "filesize");
			}
			lua_pushboolean(L, fIsDir);
			lua_setfield(L, -2, "isdir");
			lua_rawseti(L, -2, ++iEntryCount);
		} while (self->getnextdirentry(&tDirEntry));	
	}		
	
};
