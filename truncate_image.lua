
local function printf(...) print(string.format(...)) end

function truncateImage(strData)
	local iBlockSize = 1024
	local strFF = string.rep(string.char(0xff), iBlockSize)
	local iPos = strData:len()
	
	while strData:sub(iPos - iBlockSize + 1, iPos) == strFF do
		iPos = iPos - iBlockSize
	end
	
	if iPos < strData:len() then
		printf("Truncating flash image to 0x%08x bytes", iPos)
		strData = strData:sub(1, iPos)
	else
		print("Flash image not truncated")
	end
	
	return strData
end

function readBin(strFilename)
	local fd, strBin, strMsg
	fd, strMsg = io.open(strFilename, "rb")
	if fd then
		strBin = fd:read("*a")
		fd:close()
		if strBin == nil then
			strMsg = "Error reading file"
		end
	end
	
	return strBin, strMsg
end

function writeBin(strFilename, strBin)
	local fOk, fd, strMsg
	fd, strMsg = io.open(strFilename, "w+b")
	if fd == nil then
		fOk = false
	else
		fOk = true
		fd:write(strBin)
		fd:close()
	end
	return fOk, strMsg
end

strUsage = [[
Usage: lua truncate_image.lua inputfile outputfile
]]
if #arg ~=2 then
	print(strUsage)
	os.exit(1)
end

local strInputFilename = arg[1]
local strOutputFilename = arg[2]

local strBin, strMsg = readBin(strInputFilename)
if not strBin then
	print(strMsg)
	iRet = 1
else
	strBin = truncateImage(strBin)
	local fOk, strMsg = writeBin(strOutputFilename, strBin)
	
	if not fOk then
		print(strMsg)
		iRet = 1
	else
		iRet = 0
	end
end

os.exit(iRet)