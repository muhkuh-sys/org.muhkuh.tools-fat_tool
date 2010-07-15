require("fatfs")
--[[

Bauen/Extrahieren aus/zu Verzeichnisbäumen?
Verbose-Option?

The fatfs package allows to
- create a flash image containing a FAT filesystem
- get the whole image as a string, or mount an image from a string
- read/write binary data (e.g. 2nd stage loader) anywhere in the image
- read/write/delete files from/to the file system
  File operations are atomic, i.e. files are read and written as a whole.
  File-descriptor-based open/seek/read/write/close functions are not available.
- check the existence of files
- create directories in the file sytem
- set the current directory
- print a directory listing

- The directory separator is "\", escaped as "\\" under Lua
- Long file names are supported
- Invalid characters for long file names are \ / : ; * ? " < > | & + , = [ ]
                                             \ / :   * ? " < > |

Limitations:
- No wildcards
- File time/date are not supported
- no enumeration of files/directories

All functions return a data object or true if the operation succeeded.
If any error occurs, they return false or nil, but do not raise a Lua error.
Messages are printed to stdout or the Muhkuh message log. 

Create a new image/file system
	fs = fatfs.fatfs_create(sector_size, num_sectors
			[, image_size = sector_size * num_sectors, partition_offset = 0])
	Allocates memory for a flash image and initializes it to 0xff, formats and mounts a FAT file system.
	The type (FAT12/16/32) depends on the number of sectors.
	If disk size/filesystem_offset are specified, a larger buffer is allocated, and the
	file system is created at the given offset.

	Returns a fatfs object if the file system could be created, nil in case of an error.

Mount an existing file system image containing a FAT partition
	fs = fatfs.fatfs_mount(flash_image[, partition_offset])
	The partition must start with a FAT boot sector. 
	The sector size and number of sectors are read from the boot sector.
	
	Returns a fatfs object if the file system could be mounted, nil if not.

Write raw data
	bool fs:writeraw(strFileData, offset)
	Writes binary data at offset, e.g. the 2nd stage loader.
	Returns true when successful, false if any error occurred.

Read raw data
	string fs:readraw(offset, len)
	Reads len bytes of binary data starting at offset and returns the data as a string.
	Returns nil if any error occured

Get image
	string fs:getimage()
	gets the whole image as a binary string

Create a directory
	bool fs:mkdir(strPath)
	Creates a directory. 
	Returns true when successful, false if any error occurred.

Change the current directory
	bool fs:cd(strPath = "\\")

Print a directory listing
	bool fs:dir(strPath = ".", fRecursive = false)
	strPath: directory to list. Default is current directory
	fRecursive: true = list subdirs, false = list only the directory under strPath
	Returns true when successful, false if any error occurred.

Write a file
	bool fs:writefile(strFileData, strPath)
	Writes a file. If the file exists, it is overwritten.
	Returns true if successful

Read a file
	string fs:readfile(strPath)
	Reads a file, returns the contents of the file if the file is found, nil if not.

Check if a file exists
	bool fs:fileexists(strPath)
	Checks if the file exists, returns true if it exists, false if it does not exist, or if an error occurred.

Delete a file
	bool fs:deletefile(strPath)
	Deletes a file. Returns true if it could be deleted, false if it does not exist, or if an error occurred.

--]]

datadir = "D:/projekt/bsl_libfat/test/data/"

function loadbin(strName)
	local fd, msg = io.open(datadir .. strName, "rb")
	assert(fd, msg)
	local bin = fd:read("*a")
	fd:close()
	assert(bin, "error reading file " .. strName)
	assert(bin:len()>0, "file empty")
	return bin
end

function writebin(bin, strName)
	local fd, msg = io.open(datadir .. strName, "wb")
	assert(fd, msg)
	fd:write(bin)
	fd:close()
end

function printf(...) print(string.format(...)) end


fs = fatfs.fatfs_create(528, 8192-125, 528*8192, 528*125)
assert(fs, "filesystem creation failed")
fOk = fs:mkdir("SYSTEM")
assert(fOk, "mkdir failed")
fOk = fs:mkdir("PORT_0")
assert(fOk, "mkdir failed")
fOk = fs:mkdir("PORT_1")
assert(fOk, "mkdir failed")
fOk = fs:mkdir("PORT_2")
assert(fOk, "mkdir failed")
fOk = fs:mkdir("PORT_3")
assert(fOk, "mkdir failed")
fOk = fs:mkdir("PORT_4")
assert(fOk, "mkdir failed")
fOk = fs:mkdir("PORT_5")
assert(fOk, "mkdir failed")

print("copying firmware")
bin = loadbin("NTPNSNSC.NXF")
fOk = fs:writefile(bin, "PORT_0\\NTPNSNSC.NXF")
assert(fOk, "writefile failed")
print("copying script")
bin = loadbin("ledflash.lua")
fOk = fs:writefile(bin, "PORT_1\\netscrpt.lua")
assert(fOk, "writefile failed")
print("copying 2nd stage loader")
bin = loadbin("netTAP_bsl.bin")
fOk = fs:writeraw(bin, 0)
assert(fOk, "writeraw failed")

fs:dir("\\", true)

bin = fs:getimage()
assert(bin and bin:len()==528*8192, "incorrect length in flash image")
writebin(bin, "testimage.bin")

--muhkuh.TestHasFinished()
