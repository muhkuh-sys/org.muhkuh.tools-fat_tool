
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "fat_tool.h"
#include "fatfs.h"
#include "io.h"

/* read file to newly allocated buffer */
char* readFile(char* pszFilename, long *plsize) {
	char *pabBuffer = NULL;
	FILE* fd;
	size_t iBytesRead;
	long lsize;

	fd = fopen(pszFilename, "rb");
	if (fd==NULL){
		printf("Could not open file %s\n", pszFilename);
	} else {
		lsize = _filelength(_fileno(fd));

		pabBuffer = (char*)malloc(lsize);
		if (pabBuffer==NULL) {
			printf("could not allocate buffer for file\n");
		} else {
			iBytesRead = fread(pabBuffer, 1, lsize, fd);
			printf("File size: %d bytes, %d bytes read\n", lsize, iBytesRead);

			if (iBytesRead != lsize) {
				printf("error reading file\n");
				free(pabBuffer);
				pabBuffer = NULL;
			}
		}
		*plsize = lsize;
		fclose(fd);
	}
	return pabBuffer;
}

/* returns 0=success, 1=failure */
int writeFile(char* pabBuffer, size_t sizBufferSize, char* pszFilename){
	int iRes = 1;
	FILE* fd;
	size_t iBytesWritten;

	fd = fopen(pszFilename, "wb");
	if (fd==NULL){
		printf("Could not open file %s\n", pszFilename);
	} else {
		iBytesWritten = fwrite(pabBuffer, 1, sizBufferSize, fd);
		if (iBytesWritten == sizBufferSize){
			iRes = 0;
		}
		fclose(fd);
	}
	return iRes;
}

/*
	in: pszArg
	out: pulVal
	returns: 1=ok, 0=error
*/
int readULArg(char* pszArg, unsigned long* pulVal){
	if (1==sscanf(pszArg, "0x%x", pulVal)){
		return 1;
	} else if (1==sscanf(pszArg, "%d", pulVal)){
		return 1;
	} else {
		printf("Can't parse %s as an integer\n", pszArg);
		return 0;
	}
}

int readSize(char* pszArg, size_t *psize){
	if (1==sscanf(pszArg, "0x%x", psize)){
		return 1;
	} else if (1==sscanf(pszArg, "%d", psize)){
		return 1;
	} else {
		printf("Can't parse %s as an integer\n", pszArg);
		return 0;
	}
}

void print_usage(){
	printf(
		"Create and manipulate flash images with an embedded FAT file system\n"
		"Usage: fat_tool <command>...\n"
		"\n"
		"Available commands:\n"
		"-create blocksize num_blocks [imagesize FAT_offset]\n" 
		"  Create an image with a FAT file system.\n"
		"  default imagesize = blocksize * num_blocks\n"
		"  default FAT_offset = 0\n"
		"-mount file [FAT_offset]    load and mount image\n"
		"-saveimage file             write image to file\n"
		"-writeraw file offset       write binary data into image at offset\n"
		"-readraw offset len file    read binary data from image and save to file\n"
		"\n"
		"-mkdir path                 create directory\n"
		"-dir [path] [-r]            list directory\n"    
		"-cd path                    set current directory\n"
		"\n"
		"-write file destfile        copy file into file system\n"
		"-read file destfile         read file from file system\n"
		"-exists file                check if file exists\n"
		"-delete file                delete file\n" //del
		"\n"
		"The first command must be create or mount.\n"
		"File names may include a path. Path separatator is \\.\n"

		);
}
/*

returns: 0=ok, >0=error
*/

int execute_commands(int argcnt, char** argv){
	fatfs *pFS;

	size_t sizSectorSize;
	size_t sizNumBlocks;
	size_t sizImageSize;
	size_t sizOffset;
	size_t sizLen;
	long lFileSize;
	unsigned long ulSize;
	char *pszFilename;
	char *pszDestname; 
	char *pabBuffer;

	int iResult;
	bool fOk;
	bool fRecurse;

	int iArg;
	int iRemArgs;

	iArg = 1;  // skip exe filename
	pFS = NULL;

	while (iArg < argcnt) 
	{
		iRemArgs = argcnt-iArg-1; // number of args remaining after the keyword

		if (argcnt == 1 || strcmp("-help", argv[iArg])==0) {
			print_usage();
			return 0;
		}

		/* -create blocksize num_blocks [imagesize offset] */
		if (strcmp("-create", argv[iArg])==0 && iRemArgs>=2)
		{
			if (0==readSize(argv[iArg+1], &sizSectorSize)) return 1;
			if (0==readSize(argv[iArg+2], &sizNumBlocks)) return 1;
			if (iRemArgs >= 4 && argv[iArg+3][0]!='-') {
				if (0==readSize(argv[iArg+3], &sizImageSize)) return 1;
				if (0==readSize(argv[iArg+4], &sizOffset)) return 1;
				iArg += 5;
			} else {
				sizImageSize = 0;
				sizOffset = 0;
				iArg += 3;
			}
			
			if (pFS!= NULL) delete pFS;
			pFS = new fatfs(sizSectorSize, sizNumBlocks, sizImageSize, sizOffset);
			if (pFS != NULL && !pFS->checkReady()) {
				delete(pFS);
				pFS = NULL;
				return 1;
			}

		}

		/* -mount filename [offset]*/
		else if (strcmp("-mount", argv[iArg])==0 && iRemArgs>=1)
		{
			pszFilename = argv[iArg+1];
			pabBuffer = NULL;

			size_t sizOffset;
			if (iRemArgs >= 2 && argv[iArg+2][0]!='-') {
				if (0==readSize(argv[iArg+2], &sizOffset)) return 1;
				iArg += 3;
			} else {
				sizOffset = 0;
				iArg += 2;
			}
			
			pabBuffer = readFile(pszFilename, &lFileSize);
			if (pabBuffer == NULL) {
				return 1;
			} else {
				if (pFS!= NULL) delete pFS;
				pFS = new fatfs(pabBuffer, lFileSize, sizOffset);
				free(pabBuffer);
				if (pFS != NULL && !pFS->checkReady()) {
					delete(pFS);
					pFS = NULL;
					return 1;
				}
			}
		}

		else if (pFS == NULL) {
			printf("The first command must be create or mount.\n");
			return 1;
		}

		/* -saveimage filename */
		else if (strcmp("-saveimage", argv[iArg])==0 && iRemArgs>=1)
		{
			pszFilename = argv[iArg+1];
			iArg += 2;
			
			pabBuffer = pFS->getimage(&ulSize);
			if (pabBuffer == NULL) {
				printf("Failed to get image!\n");
				return 1;
			} else {
				iResult = writeFile(pabBuffer, (long) ulSize, pszFilename);
				if (iResult == 1) return 1;
			}
		}

		/* -writeraw filename offset */
		else if (strcmp("-writeraw", argv[iArg])==0 && iRemArgs>=2)
		{
			pszFilename = argv[iArg+1];
			if (0==readSize(argv[iArg+2], &sizOffset)) return 1;
			iArg += 3;

			pabBuffer = readFile(pszFilename, &lFileSize);
			if (pabBuffer == NULL) return 1;
			fOk = pFS->writeraw(pabBuffer, lFileSize, sizOffset);
			free(pabBuffer);
			if (!fOk) return 1;
		}

		/* -readraw filename offset len*/
		else if (strcmp("-readraw", argv[iArg])==0 && iRemArgs>=3)
		{
			if (0==readSize(argv[iArg+1], &sizOffset)) return 1;
			if (0==readSize(argv[iArg+2], &sizLen)) return 1;
			pszFilename = argv[iArg+3];
			iArg += 4;

			pabBuffer = pFS->readraw(sizOffset, sizLen);
			if (pabBuffer == NULL) {
				return 1;
			} else {
				iResult = writeFile(pabBuffer, (long) sizLen, pszFilename);
				if (iResult == 1) return 1;
			}
		}

		/* -mkdir dirname */
		else if(strcmp("-mkdir", argv[iArg])==0 && iRemArgs>=1)
		{
			pszFilename = argv[iArg+1];
			iArg += 2;

			fOk = pFS->mkdir(pszFilename);
			if (!fOk) return 1;
		}

		/* -cd dirname */
		else if(strcmp("-cd", argv[iArg])==0 && iRemArgs>=1)
		{
			pszFilename = argv[iArg+1];
			iArg += 2;
			fOk = pFS->cd(pszFilename);
			if (!fOk) return 1;
		}

		/* -dir [dirname] [-r] */ 
		else if(strcmp("-dir", argv[iArg])==0)
		{
			if (iRemArgs >= 1 && argv[iArg+1][0]!='-') {
				pszFilename = argv[iArg+1];
				iArg ++;
				iRemArgs --;
			} else {
				pszFilename = "\\";
			}

			if (iRemArgs >= 1 && 0==strcmp("-r", argv[iArg+1])) {
				fRecurse = true;
				iArg ++;
			} else {
				fRecurse = false;
			}

			iArg++;

			fOk = pFS->dir(pszFilename, fRecurse);
			if (!fOk) return 1;
		}


		/* -write filename destfilename */
		else if(strcmp("-write", argv[iArg])==0 && iRemArgs>=2)
		{
			pszFilename = argv[iArg+1];
			pszDestname = argv[iArg+2];
			iArg += 3;

			pabBuffer = readFile(pszFilename, &lFileSize);
			if (pabBuffer == NULL) return 1;
			fOk = pFS->writefile(pabBuffer, (size_t) lFileSize, pszDestname);
			free(pabBuffer);
			if (!fOk) return 1;		
		}

		/* -read filename destfilename */
		else if(strcmp("-read", argv[iArg])==0 && iRemArgs>=2)
		{
			pszFilename = argv[iArg+1];
			pszDestname = argv[iArg+2];
			iArg += 3;

			pabBuffer = pFS->readfile(pszFilename, &sizLen);
			if (pabBuffer == NULL) {
				return 1;
			} else {
				iResult = writeFile(pabBuffer, (long) sizLen, pszDestname);
				free(pabBuffer);
				if (iResult == 1) return 1;
			}
		}

		/* -exists filename */
		else if(strcmp("-exists", argv[iArg])==0 && iRemArgs>=1)
		{
			pszFilename = argv[iArg+1];
			iArg += 2;

			fOk = pFS->fileexists(pszFilename);
			if (fOk) {
				printf("File %s exists\n", pszFilename);
			} else {
				printf("File %s does not exist\n", pszFilename);
			}
		}

		/* -delete filename */
		else if(strcmp("-delete", argv[iArg])==0 && iRemArgs>=1)
		{
			pszFilename = argv[iArg+1];
			iArg += 2;

			pFS->deletefile(pszFilename);
		}

		else 
		{
			printf("unknown command: %s\n", argv[iArg]);
			print_usage();
			return 1;
		}
	}
		
	return 0;
}



int main(int argc, char** argv){
	if (argc>1) {
		return execute_commands(argc, argv);
	} else {
		print_usage();
		return 0;
	}



	return 0;
}
