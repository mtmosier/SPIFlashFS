#ifndef SPIFLASHFSFILE_H
#define SPIFLASHFSFILE_H

#include "SPIFlash.h"
#include "SPIFlashFS.h"
#include "Arduino.h"


class SPIFlashFS;



class SPIFlashFSFile {

	//**********************
	//*** Public Section ***
	//**********************
	public:

		//*** Initiailize
		SPIFlashFSFile(SPIFlash *flashObj, SPIFlashFS *fsObj);
		SPIFlashFSFile(SPIFlash *flashObj, SPIFlashFS *fsObj, uint16_t fileNum);
		bool isInitialized();

		//*** Load/Create Funcitons
		bool loadFile(uint16_t fileNum);
		uint32_t initNewFile(uint32_t fileSize);

		//*** Write Funcitons
		bool writeByte(uint8_t data);
		uint16_t writeByteArray(uint8_t *dataBuffer, uint16_t bufferSize);
		//bool writeString();

		//*** Read Functions
		uint8_t readByte();
		uint16_t readByteArray(uint8_t *dataBuffer, uint16_t bufferSize);
		//char* readString();
		bool available();

		//*** Erase functions
		bool eraseFile();

		//*** File position functions
		bool seek(uint32_t position);
		uint32_t getFilePosition();
		uint16_t getFileNum();
		uint32_t getFileSize();


	//***********************
	//*** Private Section ***
	//***********************
//	private:  //*** No such thing during initial dev

		uint32_t _getCurrentAddress();

		//*** Private variables
		bool               _initialized;
		SPIFlash*          _flashObj;
		SPIFlashFS*        _fsObj;
		uint16_t           _fileNum;
		uint32_t           _fileLocationAddress;
		uint32_t           _fileSize;
		volatile uint32_t  _currentPosition;

};

#endif // SPIFLASHFSFILE_H