package.cpath = package.cpath..";D:/projekt/bsl_libfat/msvc8/fat_tool/Debug/?.dll"
require("fatfs")

--------------------------------------------------------------------------
-- Create a new image/file system
--  fs = fatfs.fatfs_create(sector_size, num_sectors
-- 			[, image_size = sector_size * num_sectors, partition_offset = 0])
-- 	Allocates memory for a flash image and initializes it to 0xff, formats and mounts a FAT file system.
-- 	The type (FAT12/16/32) depends on the number of sectors.
-- 	If disk size/filesystem_offset are specified, a larger buffer is allocated, and the
-- 	file system is created at the given offset.
-- 	Returns a fatfs object if the file system could be created, nil in case of an error.

print("Testing fatfs.fatfs_create")
fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
assert(fs)
fs = fatfs.fatfs_create(528, 8192-125)
assert(fs)
-- Error conditions:
-- impossible size/malloc failure
-- sector size*num_sectors too large
fs = fatfs.fatfs_create(1000000, 1000000)
assert(fs==nil)
fs = fatfs.fatfs_create(528, 8192-125, 1000000*1000000)
assert(fs==nil)
-- image size < sector size*num_sectors
fs = fatfs.fatfs_create(528, 8192-125, 1000*528, 0)
assert(fs==nil)
-- partition_offset > image size
fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 9000*528)
assert(fs==nil)






--------------------------------------------------------------------------
-- Get image
-- 	string fs:getimage()
-- 	gets the whole image as a binary string
print()
print("Testing fs:readraw")
fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
fs_bin = fs:getimage()
assert(fs_bin and fs_bin:len()==8192*528)


--------------------------------------------------------------------------
-- Mount an existing file system image containing a FAT partition
-- 	fs = fatfs.fatfs_mount(flash_image[, partition_offset])
-- 	The partition must start with a FAT boot sector. 
-- 	The sector size and number of sectors are read from the boot sector.
-- 	
-- 	Returns a fatfs object if the file system could be mounted, nil if not.
print()
print("Testing fatfs.fatfs_mount")
fs = fatfs.fatfs_create(528, 8192-125)
assert(fs)
fs_bin = fs:getimage()
fs = fatfs.fatfs_mount(fs_bin)

fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
assert(fs)
fs_bin = fs:getimage()
fs = fatfs.fatfs_mount(fs_bin, 125*528)
assert(fs)

-- Error conditions:
-- partition_offset > image size
fs2 = fatfs.fatfs_mount(fs_bin, 9000*528)
assert(fs2==nil)
-- no boot sector at offset
fs:writeraw("xxx", 66000+0x36)
fs:writeraw("xxx", 66000+0x52)
fs_bin2 = fs:getimage()
fs2 = fatfs.fatfs_mount(fs_bin2, 125*528)
assert(fs2==nil)
fs2 = fatfs.fatfs_mount(fs_bin2)
assert(fs2==nil)

-- partition_offset + sector size * number of sectors > image size
-- other inconsistencies in file system
-- malloc failure




--------------------------------------------------------------------------
-- Write raw data
-- 	bool fs:writeraw(strFileData, offset)
-- 	Writes binary data at offset, e.g. the 2nd stage loader.
-- 	Returns true when successful, false if any error occurred.
print()
print("Testing fs:writeraw")
fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
strData = string.rep("123456789abcdef0", 3000)
fOk = fs:writeraw(strData, 1000)
assert(fOk)
fOk = fs:writeraw("", 66000)
assert(fOk)

-- Error conditions:
-- strfileData = nil
fOk = fs:writeraw(nil, 66000)
assert(not fOk)
-- strData:len() + offset > image size
fOk = fs:writeraw(strData, 8191*528)
assert(not fOk)




--------------------------------------------------------------------------
-- Read raw data
-- 	string fs:readraw(offset, len)
-- 	Reads len bytes of binary data starting at offset and returns the data as a string.
-- 	Returns nil if any error occured
print()
print("Testing fs:readraw")
fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
strData = string.rep("123456789abcdef0", 3000)
fOk = fs:writeraw(strData, 1000)
assert(fOk)
str2 = fs:readraw(1000, strData:len())
assert(str2 and str2==strData)
str2 = fs:readraw(1000, 0)
assert(str2=="")

-- Error conditions:
-- offset + len > image size
str2 = fs:readraw(8193*528, 1)
assert(str2==nil)
str2 = fs:readraw(1000*528, 8192*528)
assert(str2==nil)


--------------------------------------------------------------------------
-- Create a directory
-- 	bool fs:mkdir(strPath)
-- 	Creates a directory. 
-- 	Returns true when successful, false if any error occurred.

print()
print("Testing fs:mkdir")
fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
fOk = fs:mkdir("SYSTEM")
assert(fOk)
fOk = fs:mkdir("\\PORT_0")
assert(fOk)
fOk = fs:mkdir("\\PORT_0\\TEST")
assert(fOk)
-- long name
fOk = fs:mkdir("\\PORT_00000000")
assert(fOk)


-- Error conditions:
-- directory exists
fOk = fs:mkdir("\\PORT_0")
assert(not fOk)

-- non-existent path
fOk = fs:mkdir("\\PORT_1\\TEST")
assert(not fOk)



--------------------------------------------------------------------------
-- Change the current directory
-- 	bool fs:cd(strPath = "\\")
print()
print("Testing fs:cd")
fOk = fs:cd()
assert(fOk)
fOk = fs:cd("\\PORT_0")
assert(fOk)
fOk = fs:cd("TEST")
assert(fOk)

-- Error conditions:
-- directory does not exist
fOk = fs:cd("TEST")
assert(not fOk)
fOk = fs:cd("\\SYSTEM\\TEST")
assert(not fOk)



--------------------------------------------------------------------------
-- Print a directory listing
-- 	bool fs:dir(strPath = ".", fRecursive = false)
-- 	strPath: directory to list. Default is current directory
-- 	fRecursive: true = list subdirs, false = list only the directory under strPath
-- 	Returns true when successful, false if any error occurred.
print()
print("Testing fs:dir")

fOk = fs:dir("\\", true)
assert(fOk)
fOk = fs:cd("\\PORT_0")
assert(fOk)
fOk = fs:dir()
assert(fOk)

-- Error conditions:
-- directory does not exist
fOk = fs:dir("XXXXXXXX", true)
assert(not fOk)
fOk = fs:dir("\\XXXXXXXX", true)
assert(not fOk)
fOk = fs:dir("\\SYSTEM\\XXXXXXXX", true)
assert(not fOk)


--------------------------------------------------------------------------
-- Write a file
-- 	bool fs:writefile(strFileData, strPath)
-- 	Writes a file. Returns true if successful
print()
print("Testing fs:writefile")
fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
fOk = fs:mkdir("SYSTEM")
assert(fOk)
fOk = fs:mkdir("PORT_0")
assert(fOk)
strData = string.rep("1", 1000000) -- 1MB
fOk = fs:writefile(strData, "PORT_0\\TEST.NXF")
assert(fOk)
-- file exists: overwrite
strData = string.rep("2", 1000000) -- 1MB
fOk = fs:writefile(strData, "PORT_0\\TEST.NXF")
fOk = fs:writefile(strData, "PORT_0\\TEST.NXF")
assert(fOk) 
-- long name
fOk = fs:writefile(strData, "PORT_0\\TEST123456.NXF")
assert(fOk) 

-- Error conditions:
-- directory does not exist
fOk = fs:writefile(strData, "PORT_1\\TEST.NXF")
assert(not fOk)
-- file does not fit into free space
strData = string.rep("x", (8192-125)*528 - 1000000)
fOk = fs:writefile(strData, "PORT_0\\TEST2.NXF")
assert(not fOk)
-- file larger than file system
strData = string.rep("x", (8192-125)*528 +1000)
fOk = fs:writefile(strData, "PORT_0\\TEST3.NXF")
assert(not fOk)
-- file larger than image
strData = string.rep("x", 8192*528+1)
fOk = fs:writefile(strData, "PORT_0\\TEST4.NXF")
assert(not fOk)

fs:cd()
fs:dir("\\", true)


--------------------------------------------------------------------------
-- Read a file
-- 	string fs:readfile(strPath)
-- 	Reads a file, returns the contents of the file if the file is found, nil if not.
print()
print("Testing fs:readfile")
strData = string.rep("2", 1000000) -- 1MB
strData2 = fs:readfile("PORT_0\\TEST.NXF")
assert(strData2)
assert(strData2==strData)

-- Error conditions:
-- directory does not exist
strData2 = fs:readfile("PORT_9\\TEST0.NXF")
assert(strData2==nil)

-- file does not exist
strData2 = fs:readfile("PORT_0\\TEST0.NXF")
assert(strData2==nil)



--------------------------------------------------------------------------
-- Check if a file exists
-- 	bool fs:fileexists(strPath)
-- 	Checks if the file exists, returns true if it exists, false if it does not exist, or if an error occurred.
print()
print("Testing fs:fileexists")
fOk = fs:fileexists("PORT_0\\TEST.NXF")
assert(fOk)
fs:cd("PORT_0")
fOk = fs:fileexists("TEST.NXF")
assert(fOk)

-- Error conditions:
-- directory does not exist
fs:cd()
fOk = fs:fileexists("PORT_9\\TEST.NXF")
assert(not fOk)
fOk = fs:fileexists("\\PORT_9\\TEST.NXF")
assert(not fOk)
-- file does not exist
fOk = fs:fileexists("PORT_0\\TEST9.NXF")
assert(not fOk)



--------------------------------------------------------------------------
-- Delete a file
-- 	bool fs:deletefile(strPath)
-- 	Deletes a file. Returns true if it could be deleted, false if it does not exist, or if an error occurred.
print()
print("Testing fs:deletefile")
fOk = fs:deletefile("\\PORT_0\\TEST.NXF")
assert(fOk)

fOk = fs:fileexists("\\PORT_0\\TEST.NXF")
assert(not fOk)

strData = fs:readfile("\\PORT_0\\TEST.NXF")
assert(strData == nil)

-- Error conditions:
-- directory does not exist
fOk = fs:deletefile("\\PORT_9\\TEST.NXF")
assert(not fOk)
-- file does not exist
fOk = fs:deletefile("\\PORT_0\\TEST.NXF")
assert(not fOk)

