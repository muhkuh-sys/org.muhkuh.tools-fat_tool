22.3.2012

The fatfs package allows to
- create a flash image containing a FAT filesystem
- get the whole image as a string, or mount an image from a string
- read/write binary data (e.g. 2nd stage loader) anywhere in the image
- create directories in the file sytem
- read/write/delete files from/to the file system
- check the existence of files
- get entry type (file/directory) and file size
- list the entries in a directory
- print a directory listing

- The directory separator is "/"
- Long file names are supported
- Invalid characters for file names are 
   \ / : ; * ? " < > | & + , = [ ] and control codes (ascii 0-31)


Limitations:
- No wildcards
- no enumeration of files/directories
- File operations are atomic, i.e. files are read and written as a whole.
  File descriptor-based open/seek/read/write/close functions are not available.
- File time/date are not supported
- When printing a directory listing is, only the names of the directories
  are shown, not their parents' names.
  


Files in test directory
========================
fatfs.dll               Lua binding
make_nsc_img.lua        builds a flash image from the files in data directory
fatfs_test.lua          a collection of simple test cases
test_description.xml    runs the scripts in Muhkuh
fat_tool.exe            command line tool
test_tool.bat           Batch file to test the command line tool
data                    2nd stage loader and firmware file




Command line tool:
=======================
D:\projekt\bsl_libfat\test>..\msvc8\fat_tool\Release\fat_tool.exe
Create and manipulate flash images with an embedded FAT file system
Usage: fat_tool <command>...

Available commands:
-create blocksize num_blocks [imagesize FAT_offset]
  Create an image with a FAT file system.
  default imagesize = blocksize * num_blocks
  default FAT_offset = 0
-mount file [FAT_offset]    load and mount image
-saveimage file             write image to file
-writeraw file offset       write binary data into image at offset
-readraw offset len file    read binary data from image and save to file

-mkdir path                 create directory
-dir [path] [-r]            list directory
-cd path                    set current directory

-writefile file destfile    copy file into file system
-readfile file destfile     read file from file system
-exists file                check if file exists
-delete file                delete file

The first command must be create or mount.
File names and paths on the file system side may be written in lower or 
upper case. They are converted to upper case.
File names may include a path. The path separator in the file system is /.
On the Windows side, both \ and / are allowed.



Lua functions overview:
=========================
Operations on the flash image:
	fs = fatfs.fatfs_create(sector_size, num_sectors [, image_size = sector_size * num_sectors, partition_offset = 0])
	fs = fatfs.fatfs_mount(flash_image[, partition_offset])
	bool fs:writeraw(strFileData, offset)
	string fs:readraw(offset, len)
	string fs:getimage()
	
Directory operations:
	bool fs:mkdir(strPath)
	bool fs:cd(strPathDefaultRoot = "/")
	bool fs:dir(strPathDefaltCurrent = ".", fRecursive = false)
	
File operations:
	bool fs:writefile(strFileData, strPath)
	string fs:readfile(strPath)
	bool fs:deletefile(strPath)
	
Getting information on files and directories:
	bool fs:fileexists(strPath)
	bool fs:isfile(strPath)
	bool fs:isdir(strPath)
	filetype fs:gettype(strPath)
	int fs:getfilesize(strPath)
	table fs:getdirentries(strPath)
	


Create a new image/file system
	fs = fatfs.fatfs_create(sector_size, num_sectors
			[, image_size = sector_size * num_sectors, partition_offset = 0])
	
	Allocates memory for a flash image and initializes it to 0xff, formats and mounts a FAT file system.
	The type (FAT12/16/32) depends on the number of sectors.
	If disk size/filesystem_offset are specified, a larger buffer is allocated, and the
	file system is created at the given offset.

	Return values:
	Success											fatfs instance
	invalid filesystem size/offset					Lua error
	memory allocation failed/image too large		Lua error
	failed to format/mount file system				Lua error


Mount an existing file system image containing a FAT partition
	fs = fatfs.fatfs_mount(flash_image[, partition_offset])
	The partition must start with a FAT boot sector. 
	The sector size and number of sectors are read from the boot sector.
	
	Return values:
	Success											fatfs instance
	file system offset > image size					Lua error
	failed to identify boot sector					nil
	memory allocation failure/image too large		Lua error
	failed to mount partition						nil

	Errors may occur if the filesystem image is inconsistent.

Write raw data into the flash image
	bool fs:writeraw(strFileData, offset)
	Writes binary data at offset, e.g. the 2nd stage loader.

	Return values:
	Success											true
	offset/length exceed image size					Lua error


Read raw data from the flash image
	string fs:readraw(offset, len)
	Reads len bytes of binary data starting at offset and returns the data as a string.
	Returns nil if any error occured
	
	Return values:
	Success											string containing data from the image
	offset/length exceed image size					Lua error


Get the entire image
	string fs:getimage()
	gets the whole image as a binary string
	Return values:
	Success											string containing the whole image



Create a directory
	bool fs:mkdir(strPath)
	Creates a directory. 
	
	Return values:
	Success											true
	Any failure										Lua error


Change the current directory
	bool fs:cd(strPath = "/")

	Return values:
	Success											true
	Directory does not exist						Lua error
	Entry does exist but is a file					Lua error
	

Write a file
	bool fs:writefile(strFileData, strPath)
	Writes a file. If the file exists, it is overwritten.
	
	Return values:
	Success											true
	Any error										Lua error


Read a file
	string fs:readfile(strPath)
	Reads a file, returns the contents of the file if the file is found, nil if not.

	Return values:
	Success											string with the contents of the file
	Any error										Lua error
	


Delete a file
	bool fs:deletefile(strPath)
	Deletes a file or an empty directory. 

	Return values:
	Success											true
	File does not exist								Lua error
	StrPath is a non-empty directory				Lua error


Check if a file or directory with the given name exists
	bool fs:fileexists(strPath)
	Checks if the file exists, returns true if it exists, false if it does not exist, or if an error occurred.
	
	Return values:
	File/directory exists							true
	File/directory does not exist					false


Get the type of a dir entry
	fs:gettype(strPath)

	Return values:
	No entry under strPath							fatfs.fatfs_TYPE_NONE
	strPath is a file								fatfs.fatfs_TYPE_FILE
	strPath is a directory							fatfs.fatfs_TYPE_DIRECTORY
	

	fs:isfile(strPath)
	returns true if a file exists under the given path.
	
	Return values:
	No entry under strPath							false
	strPath is a file								true
	strPath is a directory							false
	
	fs:isdir(strPath)
	returns true if a directory exists under the given path.
	
	Return values:
	No entry under strPath							false
	strPath is a file								false
	strPath is a directory							true


Get the file size
	int fs:getfilesize(strPath)
	Returns the file size, or -1 if the file could not be found.
	
	Return values:
	strPath exists and is a file					File size
	strPath is a directory							nil
	strPath not found								nil


Get the entries in a directory
	table fs:getdirentries(strPath)
	
	The entries are indexed with numerical indices.
	Each entry is itself a table with the following fields:
	index     type  
	----------------------------------------------------------
	name      string    file/dir name
	isdir     boolean   indicates if the entry is a directory
	filesize  number    files size for files

	Return values:
	Success											list of entries
	Directory does not exist or is not a directory	nil

	function printdirentries(t)
		for i, entry in ipairs(t) do
			if entry.isdir then
				print(string.format("<Dir>    %s", entry.name))
			else
				print(string.format("%8d %s", entry.filesize, entry.name))
			end
		end
	end
	printdirentries(fs::getdirentries("/"))


Print a directory listing
	bool fs:dir([strPath = "."[, fRecursive = false]])
	strPath: directory to list. Default is current directory
	fRecursive: true = list subdirs, false = list only the directory under strPath

	Return values:
	Success											true
	StrPath doesnot exist or is not a directory		false
	Any other errors								Lua error

