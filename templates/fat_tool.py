# -*- coding: utf-8 -*-

from SCons.Script import *

import array
import re
import subprocess


def flashimage_action(target, source, env):
	# Get the path of the file.
	strCfgPath = os.path.dirname(source[0].get_path())
	
	# Get the absolute path of the target file.
	strTargetPath = os.path.abspath(target[0].get_path())
	
	# Get the complete file contents.
	strCfg = source[0].get_contents()
	
	aCmds = []
	aCmds.append(env['FAT_TOOL'])
	
	# Leave out blank lines or lines starting with '#'.
	for strLine in strCfg.splitlines():
		if len(strLine)!=0 and strLine[0]!='#':
			aCmds.extend(strLine.split())
	
	aCmds.extend(['-saveimage', strTargetPath])
	
	# Get the current working directory. This must be restored at the end
	# of the function.
	strOldCwd = os.getcwd()
	
	# Change to the folder of the sourcefile.
	os.chdir(os.path.abspath(strCfgPath))
	strLog = subprocess.check_output(aCmds)
	
	# Move back to the old current working directory.
	os.chdir(strOldCwd)
	
	if 'FAT_TOOL_LOGFILE' in env:
		strPathLogFile = env['FAT_TOOL_LOGFILE']
		tLogFile = open(strPathLogFile, 'wt')
		tLogFile.write(strLog)
		tLogFile.close()
	
	return None



def flashimage_emitter(target, source, env):
	# Get the path of the file.
	strCfgPath = os.path.dirname(source[0].get_path())
	
	# Get the complete file contents.
	strCfg = source[0].get_contents()
	
	# Search for all 'writeraw' options.
	for tMatch in re.finditer('-writeraw\s+(\S+)\s+', strCfg):
		strPath = os.path.abspath(os.path.join(strCfgPath, tMatch.group(1)))
		Depends(target, strPath)
	
	# Search for all 'writefile' options.
	for tMatch in re.finditer('-writefile\s+(\S+)\s+', strCfg):
		strPath = os.path.abspath(os.path.join(strCfgPath, tMatch.group(1)))
		Depends(target, strPath)
	
	if 'FAT_TOOL_LOGFILE' in env:
		strPathLogFile = env['FAT_TOOL_LOGFILE']
		Clean(target, strPathLogFile)
	
	return target, source



def flashimage_string(target, source, env):
	return 'FlashImage %s' % target[0].get_path()



def truncate_action(target, source, env):
	tFileBin = None
	tFileCfg = None
	
	# Loop over all sources and find the bin and cfg file.
	for tSource in source:
		(strDummy,strExt) = os.path.splitext(tSource.get_path())
		if strExt=='.ftc':
			if not tFileCfg is None:
				raise Exception('Two config files specified!')
			tFileCfg = tSource
		else: 
			if not tFileBin is None:
				raise Exception('Two bin files specified!')
			tFileBin = tSource
	
	if not 'FAT_TOOL_TRUNCATE_BLOCKSIZE' in env:
		ulBlockSize = None
	else:
		ulBlockSize = env['FAT_TOOL_TRUNCATE_BLOCKSIZE']
	
	if ulBlockSize is None:
		# Get the block size. The default is 1024.
		ulBlockSize = 1024
		if not tFileCfg is None:
			# Get the complete file contents.
			strCfg = tFileCfg.get_contents()
			tMatch = re.match('-create\s+(\d+)\s+', strCfg)
			if not tMatch is None:
				ulBlockSize = long(tMatch.group(1))
			else:
				tMatch = re.match('-create\s+0[xX]([0-9a-fA-F]+)\s+', strCfg)
				if not tMatch is None:
					ulBlockSize = long(tMatch.group(1), 16)
				else:
					raise Exception('Failed to extract the block size from the configuration!')
	else:
		ulBlockSize = long(env['FAT_TOOL_TRUNCATE_BLOCKSIZE'])
	
	# Get the file size.
	ulFileSize = tFileBin.get_size()
	# The file must contain at least one block.
	if ulFileSize<ulBlockSize:
		raise Exception('The file size %d is too small for one block of the size %d!' % (ulFileSize,ulBlockSize))
	# The file size must be a multiple of the block size.
	if (ulFileSize % ulBlockSize)!=0:
		raise Exception('The file size %d is no multiple of the block size %d!' % (ulFileSize,ulBlockSize))
	
	# Read the complete file into an array object.
	tFile = open(tFileBin.get_path(), 'rb')
	tFileData = array.array('B')
	tFileData.fromfile(tFile, ulFileSize)
	tFile.close()
	
	# Generate an empty block.
	atEmptyBlock = array.array('B', [255]*ulBlockSize)
	
	# Set the default size of the image to the absolute minimum of 1 block.
	ulImageEnd = ulBlockSize
	
	# Loop over all blocks starting at the last one down to the 2nd block.
	for ulOffset in range(ulFileSize-ulBlockSize, ulBlockSize, -ulBlockSize):
		# Extract the block.
		aBlock = tFileData[ulOffset:ulOffset+ulBlockSize]
		# Check if this block is empty.
		if aBlock!=atEmptyBlock:
			ulImageEnd = ulOffset+ulBlockSize
			break
	
	# Cut down the image.
	atCutDownImage = tFileData[0:ulImageEnd]
	
	# Write the cut-down image to the target file.
	tOutFile = open(target[0].get_path(), 'wb')
	atCutDownImage.tofile(tOutFile)
	tOutFile.close()
	
	return None



def truncate_emitter(target, source, env):
	if len(source)!=1 and len(source)!=2:
		raise Exception('There must be 1 or 2 sources for this rule, one bin file and optinally one config.')
	
	if 'FAT_TOOL_TRUNCATE_BLOCKSIZE' in env:
		Depends(target, SCons.Node.Python.Value(env['FAT_TOOL_TRUNCATE_BLOCKSIZE']))
	
	return target, source



def truncate_string(target, source, env):
	return 'TruncateFlashImage %s' % target[0].get_path()



def ApplyToEnv(env):
	strVersion = '1.0.0'
	strMbsRelease = '1'
	
	env['FAT_TOOL'] = os.path.join( os.path.dirname(os.path.realpath(__file__)), 'fat_tool-%s_%s'%(strVersion,strMbsRelease), 'fat_tool' )
	
	flashimage_act = SCons.Action.Action(flashimage_action, flashimage_string)
	flashimage_bld = Builder(action=flashimage_act, emitter=flashimage_emitter, suffix='.bin', src_suffix='.flc', single_source = 1)
	env['BUILDERS']['FlashImage'] = flashimage_bld
	
	truncate_act = SCons.Action.Action(truncate_action, truncate_string)
	truncate_bld = Builder(action=truncate_act, emitter=truncate_emitter, suffix='.bin')
	env['BUILDERS']['TruncateFlashImage'] = truncate_bld
