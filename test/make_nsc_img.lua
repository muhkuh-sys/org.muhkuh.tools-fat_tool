
package.cpath = package.cpath..";D:/projekt/bsl_libfat/msvc8/fat_tool/Release/?.dll"
require("fatfs")
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

-- Create the flash image with a FAT file system
-- filesystem system size is 8192-125 sectors with 528 bytes each
-- flash size is size 8192*528 bytes
-- file system starts at offset 528*125 (66000)
fs = fatfs.fatfs_create(528, 8192-125, 528*8192, 528*125)

-- Create the standard directories
fs:mkdir("SYSTEM")
fs:mkdir("PORT_0")
fs:mkdir("PORT_1")
fs:mkdir("PORT_2")
fs:mkdir("PORT_3")
fs:mkdir("PORT_4")
fs:mkdir("PORT_5")

-- copy the firmware into the filesystem
print("copying firmware")
bin = loadbin("NTPNSNSC.NXF")
fs:writefile(bin, "PORT_0/NTPNSNSC.NXF")
print("copying script")
bin = loadbin("ledflash.lua")
fs:writefile(bin, "PORT_1/netscrpt.lua")

-- Copy the 2nd stage loader to the start of the flash image
print("copying 2nd stage loader")
bin = loadbin("netTAP_bsl.bin")
fs:writeraw(bin, 0)

-- List the filesystem
fs:dir("/", true)

-- Write the flash image to a file
bin = fs:getimage()
assert(bin and bin:len()==528*8192, "incorrect length in flash image")
writebin(bin, "testimage.bin")

-- For faster flashing, truncate the image to the part which is different from 0xff
local blocklen = 1024
local strFF = string.rep(string.char(0xff), blocklen)
local l = bin:len()
while (l>1 and bin:sub(l-blocklen+1, l) == strFF) do l = l - blocklen end
printf("old len: %d  new len: %d", bin:len(), l)
bin = bin:sub(1, l)

--muhkuh.TestHasFinished()
