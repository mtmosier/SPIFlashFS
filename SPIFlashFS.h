#ifndef SPIFLASHFS_H
#define SPIFLASHFS_H

#include "Arduino.h"
#include "SPIFlash.h"
#include "SPIFlashFSFile.h"


class SPIFlashFSFile;



class SPIFlashFS {

	//**********************
	//*** Public Section ***
	//**********************
	public:

		//*** Initiailize
		SPIFlashFS(SPIFlash *flashObj);
		bool isInitialized();

		//*** Load/Create Funcitons
		bool removeFileEntry(uint16_t fileNum);
		bool loadFileInfo(uint16_t fileNum, uint32_t *address, uint32_t *fileSize);
		bool loadFileInfo(uint16_t fileNum, uint32_t *address, uint32_t *fileSize, uint8_t *status);
		bool allocateNewFile(uint32_t fileSize, uint32_t *address, uint16_t *fileNum);

		//*** Convenience functions
		SPIFlashFSFile* openFile(uint16_t fileNum);
		SPIFlashFSFile* createFile(uint32_t fileSize);

		//*** General functions
		uint16_t getFileCount();
		uint32_t getCapacity();

		//*** Testing functions
		bool isFATEntrySane(uint32_t address, uint32_t fileSize, uint8_t status, bool considerErasedAsValid = false);


	//***********************
	//*** Private Section ***
	//***********************
//	private:  //*** No such thing during initial dev

		//*** Private variables
		bool      _initialized;
		uint16_t  _fileCount;
		uint32_t  _capacity;
		SPIFlash* _flashObj;
		uint16_t  _sectorsReservedForFAT;
		uint16_t  _sectorsReservedForErase;
		uint32_t  _maxFileEntriesPossible;

		bool _loadFileCount();
		bool _eraseByteRange(uint32_t startAddress, uint32_t bytesToErase);
		bool _copyByteRange(uint32_t fromAddress, uint32_t bytesToCopy, uint32_t toAddress);
};

#endif // SPIFLASHFSFILE_H
