
package.cpath = package.cpath..";D:/projekt/bsl_libfat/msvc8/fat_tool/Debug/?.dll"
require("fatfs")

function assertFS(...)
	return assertType("userdata", ...)
end

function assertString(...)
	return assertType("string", ...)
end

function assertNil(...)
	return assertVal(nil, ...)
end

function assertTrue(...)
	return assertVal(true, ...)
end

function assertFalse(...)
	return assertVal(false, ...)
end


function assertFail(...)
	local info = debug.getinfo(2)
	io.write(info.currentline, ": ")
	local fRes, strMsg = pcall(...)
	if fRes then 
		-- error("call did not fail as expected")
	else
		print(info.currentline .. ": failed with message: ", strMsg)
	end
end

-----------------------------------------------------------
function assertType(t, ...)
	local info = debug.getinfo(3)
	io.write(info.currentline, ": ")
	local fRes, x = pcall(...)
	if fRes then
		if type(x)==t then
			-- print("returned", x)
			print("")
			return x
		else
			error(string.format("expected type: %s, returned: %s", t, type(x)))
		end
	else
		error(string.format("call failed: %s", x or "<nil>"))
	end
end

function assertVal(v, ...)
	local info = debug.getinfo(3)
	io.write(info.currentline, ": ")
	local fRes, x = pcall(...)
	if fRes then
		if x==v then
			-- print("returned", x)
			return x
		else
			error(string.format("expected: %s, returned: %s", tostring(v), tostring(x)))
		end
	else
		error(string.format("call failed: %s", x or "<nil>"))
	end
end


--------------------------------------------------------------------------
print("Testing fatfs.fatfs_create")

fs = assertFS(fatfs.fatfs_create, 528, 8192-125, 8192*528, 125*528)
fs = assertFS(fatfs.fatfs_create, 528, 8192-125)


-- Error conditions:
-- impossible size/malloc failure
-- sector size*num_sectors too large
assertFail(fatfs.fatfs_create, 1000000, 1000000)
assertFail(fatfs.fatfs_create, 528, 8192-125, 1000000*1000000)
-- image size < sector size*num_sectors
assertFail(fatfs.fatfs_create, 528, 8192-125, 1000*528, 0)
-- partition_offset > image size
assertFail(fatfs.fatfs_create, 528, 8192-125, 8192*528, 9000*528)






--------------------------------------------------------------------------
print()
print("Testing fs:getimage")

fs = assertFS(fatfs.fatfs_create, 528, 8192-125, 8192*528, 125*528)
fs_bin = assertString(fs.getimage, fs)
assert(fs_bin and fs_bin:len()==8192*528)


--------------------------------------------------------------------------
print()
print("Testing fatfs.fatfs_mount")

fs = assertFS(fatfs.fatfs_create, 528, 8192-125)
fs_bin = fs:getimage()
fs = assertFS(fatfs.fatfs_mount, fs_bin)

fs = assertFS(fatfs.fatfs_create, 528, 8192-125, 8192*528, 125*528)
fs_bin = fs:getimage()
fs = assertFS(fatfs.fatfs_mount, fs_bin, 125*528)

-- Error conditions:
-- partition_offset > image size
assertFail(fatfs.fatfs_mount, fs_bin, 9000*528)
-- no boot sector at offset
fs:writeraw("xxx", 66000+0x36)
fs:writeraw("xxx", 66000+0x52)
fs_bin2 = fs:getimage()
assertNil(fatfs.fatfs_mount, fs_bin2, 125*528)
assertNil(fatfs.fatfs_mount, fs_bin2)




--------------------------------------------------------------------------
print()
print("Testing fs:writeraw")

fs = assertFS(fatfs.fatfs_create, 528, 8192-125, 8192*528, 125*528)
strData = string.rep("123456789abcdef0", 3000)
assertTrue(fs.writeraw, fs, strData, 1000)
assertTrue(fs.writeraw, fs, "", 66000)

-- Error conditions:
-- strfileData = nil
assertFail(fs.writeraw, fs, nil, 66000)
-- strData:len() + offset > image size
assertFail(fs.writeraw, fs, strData, 8191*528)
-- missing arguments
assertFail(fs.writeraw, fs, nil, nil)
assertFail(fs.writeraw, fs, nil)
assertFail(fs.writeraw, fs)


--------------------------------------------------------------------------
print()
print("Testing fs:readraw")

fs = assertFS(fatfs.fatfs_create, 528, 8192-125, 8192*528, 125*528)
strData = string.rep("123456789abcdef0", 3000)
assertTrue(fs.writeraw, fs, strData, 1000)
str2 = assertString(fs.readraw, fs, 1000, strData:len())
assert(str2 and str2==strData)
str2 = assertString(fs.readraw, fs, 1000, 0)
assert(str2=="")

-- Error conditions:
-- offset + len > image size
assertFail(fs.readraw, fs, 8193*528, 1)
assertFail(fs.readraw, fs, 1000*528, 8192*528)
-- missing arguments
assertFail(fs.readraw, fs, nil, 10000)
assertFail(fs.readraw, fs, 10000, nil)

--------------------------------------------------------------------------
print()
print("Testing fs:mkdir")

fs = assertFS(fatfs.fatfs_create, 528, 8192-125, 8192*528, 125*528)
assertTrue(fs.mkdir, fs, "SYSTEM")
assertTrue(fs.mkdir, fs, "/PORT_0")
assertTrue(fs.mkdir, fs, "/PORT_0/TEST")

-- long name
assertTrue(fs.mkdir, fs, "/PORT_00000000")

strBin = string.rep("1234567890", 1000)
assertTrue(fs.writefile, fs, strBin, "/PORT_0/FILE.BIN")

-- Error conditions:
-- directory exists
assertFail(fs.mkdir, fs, "/PORT_0")
-- a file with the same name exists
assertFail(fs.mkdir, fs, "/PORT_0/FILE.BIN")
assertFail(fs.mkdir, fs, "/PORT_0/FILE.BIN/SUBDIR")

-- non-existent path
assertFail(fs.mkdir, fs, "/PORT_1/TEST")
-- missing arguments
assertFail(fs.mkdir, fs, nil)


--------------------------------------------------------------------------
print()
print("Testing fs:cd")

assertTrue(fs.cd, fs)
assertTrue(fs.cd, fs, "/PORT_0")
assertTrue(fs.cd, fs, "TEST")

-- Error conditions:
-- directory does not exist
assertFail(fs.cd, fs, "TEST")
assertFail(fs.cd, fs, "/SYSTEM/TEST")
-- cd to a file
assertFail(fs.cd, fs, "/PORT_0/FILE.BIN")
-- missing arguments
assertFail(fs.cd, fs, nil)
--------------------------------------------------------------------------
print()
print("Testing fs:dir")

assertTrue(fs.dir, fs, "/", true)
assertTrue(fs.cd, fs, "/PORT_0")
assertTrue(fs.dir, fs)

-- Error conditions:
-- directory does not exist
assertFalse(fs.dir, fs, "XXXXXXXX", true)
assertFalse(fs.dir, fs, "/XXXXXXXX", true)
assertFalse(fs.dir, fs, "/SYSTEM/XXXXXXXX", true)
-- missing arguments
assertFail(fs.dir, fs, nil)


--------------------------------------------------------------------------
print()
print("Testing fs:writefile")

fs = fatfs.fatfs_create(528, 8192-125, 8192*528, 125*528)
fOk = fs:mkdir("SYSTEM")
assert(fOk)
fOk = fs:mkdir("PORT_0")
assert(fOk)
strData = string.rep("1", 1000000) -- 1MB
fOk = fs:writefile(strData, "PORT_0/TEST.NXF")
assert(fOk)
-- file exists: overwrite
strData = string.rep("2", 1000000) -- 1MB
fOk = fs:writefile(strData, "PORT_0/TEST.NXF")
fOk = fs:writefile(strData, "PORT_0/TEST.NXF")
assert(fOk) 
-- long name
fOk = fs:writefile(strData, "PORT_0/TEST123456.NXF")
assert(fOk) 

-- Error conditions:
-- directory does not exist
assertFail(fs.writefile, fs, strData, "PORT_1/TEST.NXF")
-- file does not fit into free space
strData = string.rep("x", (8192-125)*528 - 1000000)
assertFail(fs.writefile, fs, strData, "PORT_0/TEST2.NXF")
-- file larger than file system
strData = string.rep("x", (8192-125)*528 +1000)
assertFail(fs.writefile, fs, strData, "PORT_0/TEST3.NXF")
-- file larger than image
strData = string.rep("x", 8192*528+1)
assertFail(fs.writefile, fs, strData, "PORT_0/TEST4.NXF")


-- missing arguments
assertFail(fs.writefile, fs, "abcd", nil)
assertFail(fs.writefile, fs, "abcd")
assertFail(fs.writefile, fs, nil, 123)
assertFail(fs.writefile, fs)


fs:cd()
fs:dir("/", true)


--------------------------------------------------------------------------
print()
print("Testing fs:readfile")

strData = string.rep("2", 1000000) -- 1MB
strData2 = fs:readfile("PORT_0/TEST.NXF")
assert(strData2)
assert(strData2==strData)

-- Error conditions:
-- directory does not exist
assertFail(fs.readfile, fs, "PORT_9/TEST0.NXF")

-- file does not exist
assertFail(fs.readfile, fs, "PORT_0/TEST0.NXF")



--------------------------------------------------------------------------
print()
print("Testing fs:fileexists")

assertTrue(fs.fileexists, fs, "PORT_0/TEST.NXF")
fs:cd("PORT_0")
assertTrue(fs.fileexists, fs, "TEST.NXF")

-- Error conditions:
-- directory does not exist
fs:cd()
assertFalse(fs.fileexists, fs, "PORT_9/TEST.NXF")
assertFalse(fs.fileexists, fs, "/PORT_9/TEST.NXF")
-- file does not exist
assertFalse(fs.fileexists, fs, "PORT_0/TEST9.NXF")

-- missing arguments
assertFail(fs.fileexists, fs, nil)
assertFail(fs.fileexists, fs)

--------------------------------------------------------------------------
print()
print("Testing fs:getfilesize")

l = fs:getfilesize("PORT_0/TEST.NXF")
assert(l==1000000)

-- file does not exist
assertFail(fs.getfilesize, fs, "PORT_1/TEST9.NXF")
assertFail(fs.getfilesize, fs, "PORT_1")
assertFail(fs.getfilesize, fs, "PORT_10/TEST9.NXF")


--------------------------------------------------------------------------
print()
print("Testing fs:gettype/isdir/isfile")

assert(fatfs.fatfs_TYPE_NONE)
assert(fatfs.fatfs_TYPE_FILE)
assert(fatfs.fatfs_TYPE_DIRECTORY)

-- file
t = fs:gettype("PORT_0/TEST.NXF")
assert(t == fatfs.fatfs_TYPE_FILE)
-- directory
t = fs:gettype("PORT_0")
assert(t == fatfs.fatfs_TYPE_DIRECTORY)

-- non-existent file
t = fs:gettype("PORT_0/TEST99.NXF")
assert(t == fatfs.fatfs_TYPE_NONE)
-- non-existent directory
t = fs:gettype("PORT_99")
assert(t == fatfs.fatfs_TYPE_NONE)
-- missing arguments
assertFail(fs.gettype, fs, nil)
assertFail(fs.gettype, fS)

-- file
assertTrue(fs.isfile, fs, "PORT_0/TEST.NXF")
-- directory
assertFalse(fs.isfile, fs, "PORT_0")
-- non-existent file
assertFalse(fs.isfile, fs,"PORT_0/TEST99.NXF")
-- non-existent directory
assertFalse(fs.isfile, fs,"PORT_99")
-- missing arguments
assertFail(fs.isfile, fs, nil)
assertFail(fs.isfile, fs)

-- file
assertFalse(fs.isdir, fs,"PORT_0/TEST.NXF")
-- directory
assertTrue(fs.isdir, fs,"PORT_0")
-- non-existent file
assertFalse(fs.isdir, fs,"PORT_0/TEST99.NXF")
-- non-existent directory
assertFalse(fs.isdir, fs,"PORT_99")
-- missing arguments
assertFail(fs.isdir, fs, nil)
assertFail(fs.isdir, fs)

--------------------------------------------------------------------------
print()
print("Testing fs:getdirentries")


function printdirentries(t)
	for i, entry in ipairs(t) do
		if entry.isdir then
			print(string.format("<Dir>    %s", entry.name))
		else
			print(string.format("%8d %s", entry.filesize, entry.name))
		end
	end
end

-- proper directory
l = fs:getdirentries("/PORT_0")
assert(type(l)=="table")
printdirentries(l)

-- empty directory
l = fs:getdirentries("SYSTEM")
assert(type(l)=="table")
printdirentries(l)

-- file
assertNil(fs.getdirentries, fs, "PORT_0/TEST99.NXF")

-- non-existent directory
assertNil(fs.getdirentries, fs, "/PORT_9")
-- missing arguments
assertFail(fs.getdirentries, fs, nil)
assertFail(fs.getdirentries, fs)


--------------------------------------------------------------------------
print()
print("Testing fs:deletefile")
assertTrue(fs.deletefile, fs, "/PORT_0/TEST.NXF")

assertFalse(fs.fileexists, fs, "/PORT_0/TEST.NXF")

assertFail(fs.readfile, fs, "/PORT_0/TEST.NXF")

-- Error conditions:
-- directory does not exist
assertFail(fs.deletefile, fs, "/PORT_9/TEST.NXF")
-- file does not exist
assertFail(fs.deletefile, fs, "/PORT_0/TEST.NXF")

