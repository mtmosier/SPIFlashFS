#include "SPIFlashFS.h"


/*

1 page = 256 bytes
16 pages = 1 sector (4096 bytes)

Erase can happen on sector level minimum.

Sectors 0 - 15 (Pages 0 - 255) reserved for the FAT
Sector 16 (Pages 256 - 271) reserved for data copying
Sector 17 (Page 272) begins the data section


Each FAT entry is 9 bytes

ffffffff ffffffff ff
|------| |------| ||
Addrress Size     Status

Status is 0xff for normal, 0x00 for deleted

*/



//*** We assume this chip is either erased and fresh, or previously initialized using SPIFlashFS
//*** Calling this with a chip used by another application without erasing first will have unpredicable results

SPIFlashFS::SPIFlashFS(SPIFlash *flashObj)
{
	_flashObj = flashObj;
	_initialized = false;
	_capacity = _flashObj->getCapacity();

	//*** I might change these depending on capacity of the chip (less for 1mb, more for 16mb+)
	_sectorsReservedForFAT = 16;
	_sectorsReservedForErase = 1;
	_maxFileEntriesPossible = floor((uint32_t) _sectorsReservedForFAT * 4096 / 9);

	_loadFileCount();
}



bool
SPIFlashFS::isInitialized()
{
	return _initialized;
}



bool
SPIFlashFS::removeFileEntry(uint16_t fileNum)
{
	bool rtnVal;
	uint32_t seekAddress;

	if (!isInitialized())  return false;


	//*** Set the status byte to 0, indicating the file is deleted
	seekAddress = (uint32_t) fileNum * 9 + 8;
	rtnVal = _flashObj->writeByte(seekAddress, 0);

	return rtnVal;
}



bool
SPIFlashFS::loadFileInfo(uint16_t fileNum, uint32_t *address, uint32_t *fileSize)
{
	uint8_t status;
	bool result;

	if (!isInitialized())  return false;


	result = loadFileInfo(fileNum, address, fileSize, &status);

	if (!result || status == 0) {
		*address = 0;
		*fileSize = 0;
		result = false;
	}

	return result;
}



bool
SPIFlashFS::loadFileInfo(uint16_t fileNum, uint32_t *address, uint32_t *fileSize, uint8_t *status)
{
	uint32_t seekAddress;
	bool rtnVal = false;

	if (fileNum <= _maxFileEntriesPossible) {

		//*** Get the address first
		seekAddress = (uint32_t) fileNum * 9;
		*address = _flashObj->readULong(seekAddress);

		//*** Now read the file size
		seekAddress += 4;
		*fileSize = _flashObj->readULong(seekAddress);

		//*** Last, the status byte
		seekAddress += 4;
		*status = _flashObj->readByte(seekAddress);

		rtnVal = isFATEntrySane(*address, *fileSize, *status);
	}

	return rtnVal;
}



bool
SPIFlashFS::allocateNewFile(uint32_t fileSize, uint32_t *address, uint16_t *fileNum)
{
	uint32_t seekAddress;
	uint32_t lastFileAddress;
	uint32_t lastFileSize;
	uint16_t fileNumTmp;
	uint8_t  status;
	bool rtnVal = false;

	if (!isInitialized())  return false;

	if (_fileCount < _maxFileEntriesPossible) {
		if (_fileCount > 0) {

			fileNumTmp = _fileCount - 1;
			loadFileInfo(fileNumTmp, &lastFileAddress, &lastFileSize, &status);

			if (status == 0) {
				//*** The last file was deleted. Appropriate it's adress space
				*address = lastFileAddress;
			} else {
				//*** Place our starting address on the next sector boundary
				*address = ceil((lastFileAddress + lastFileSize) / 4096.0) * 4096;
			}

			if (_capacity - *address <= fileSize) {
				*address = 0;
			} else {
				*fileNum = _fileCount;
				rtnVal = true;
			}

		} else {

			*fileNum = 0;
			*address = (uint32_t) (_sectorsReservedForFAT +  _sectorsReservedForErase) * 4096;
			rtnVal = true;
		}

		if (rtnVal) {
			//*** Write the FAT entry

			//*** Write the address
			seekAddress = (uint32_t) (*fileNum) * 9;
			_flashObj->writeULong(seekAddress, *address);

			//*** Write the file size
			seekAddress += 4;
			_flashObj->writeULong(seekAddress, fileSize);

			//*** Write status byte
			//*** Status byte is 0xff by default, which is the non-written value.  Leave it as is 
			seekAddress += 4;
			_flashObj->writeByte(seekAddress, 0xff);

			//*** UPdate the file count
			_fileCount += 1;
		}
	}

	return rtnVal;
}



SPIFlashFSFile*
SPIFlashFS::openFile(uint16_t fileNum)
{
	SPIFlashFSFile *fileObj;

	if (!isInitialized())  return NULL;


	fileObj = new SPIFlashFSFile(_flashObj, this, fileNum);

	if (!fileObj->isInitialized()) {
		delete fileObj;
		fileObj = NULL;
	}

	return fileObj;
}



SPIFlashFSFile*
SPIFlashFS::createFile(uint32_t fileSize)
{
	SPIFlashFSFile *fileObj;

	if (!isInitialized())  return NULL;


	fileObj = new SPIFlashFSFile(_flashObj, this);
	fileObj->initNewFile(fileSize);

	if (!fileObj->isInitialized()) {
		delete fileObj;
		fileObj = NULL;
	}

	return fileObj;
}



uint16_t
SPIFlashFS::getFileCount()
{
	return _fileCount;
}



uint32_t
SPIFlashFS::getCapacity()
{
	return _capacity;
}



bool
SPIFlashFS::_eraseByteRange(uint32_t startAddress, uint32_t bytesToErase)
{

	if (!isInitialized())  return false;


	return false;
}



bool
SPIFlashFS::_copyByteRange(uint32_t fromAddress, uint32_t bytesToCopy, uint32_t toAddress)
{

	if (!isInitialized())  return false;


	return false;
}



bool
SPIFlashFS::_loadFileCount()
{
	uint32_t lowBound, highBound, testFileNum;
	uint32_t fileAddress, fileSize;
	uint8_t  fileStatus;
	bool invalidFAT = false;

	_initialized = false;

	lowBound = 0;
	highBound = _maxFileEntriesPossible - 1;

	while (lowBound < highBound) {

		testFileNum = (uint32_t) ((highBound - lowBound) / 2);

		loadFileInfo(testFileNum, &fileAddress, &fileSize, &fileStatus);

		if (fileAddress == 0xffffffff) {
			//*** No entry found

			if (testFileNum > 0) {
				loadFileInfo(testFileNum - 1, &fileAddress, &fileSize, &fileStatus);
				if (fileAddress != 0xffffffff) {

					if (!isFATEntrySane(fileAddress, fileSize, fileStatus)) {
						//*** FAILURE
						//*** Invalid data, chip must be erased before use
						if (Serial) {
							Serial.print("File entry ");
							Serial.print(testFileNum - 1);
							Serial.println(" is invalid");
							Serial.print("@");
							Serial.print(fileAddress);
							Serial.print(" ");
							Serial.print(fileSize);
							Serial.print("B  ");
							Serial.println(fileStatus);
						}
						break;
					}

					_fileCount = testFileNum;
					_initialized = true;
					break;
				}
			} else {
				_fileCount = testFileNum;
				_initialized = true;
				break;
			}

			highBound = testFileNum - 1;

		} else {
			//*** Entry found

			if (!isFATEntrySane(fileAddress, fileSize, fileStatus)) {

				//*** FAILURE
				//*** Invalid data, chip must be erased before use
				if (Serial) {
					Serial.print("File entry ");
					Serial.print(testFileNum);
					Serial.println(" is invalid");
					Serial.print("@");
					Serial.print(fileAddress);
					Serial.print(" ");
					Serial.print(fileSize);
					Serial.print("B  ");
					Serial.println(fileStatus);
				}
				break;
			}

			if (testFileNum < _maxFileEntriesPossible - 1) {
				loadFileInfo(testFileNum + 1, &fileAddress, &fileSize, &fileStatus);

				if (fileAddress == 0xffffffff) {
					_fileCount = testFileNum + 1;
					_initialized = true;
					break;

				} else if (!isFATEntrySane(fileAddress, fileSize, fileStatus)) {

					//*** FAILURE
					//*** Invalid data, chip must be erased before use
					if (Serial) {
						Serial.print("File entry ");
						Serial.print(testFileNum + 1);
						Serial.println(" is invalid");
						Serial.print("@");
						Serial.print(fileAddress);
						Serial.print(" ");
						Serial.print(fileSize);
						Serial.print("B  ");
						Serial.println(fileStatus);
					}
					break;
				}

			} else {
				_fileCount = testFileNum + 1;
				_initialized = true;
				break;
			}

			lowBound = testFileNum + 1;
		}	
	}

	return _initialized;
}



bool
SPIFlashFS::isFATEntrySane(uint32_t address, uint32_t fileSize, uint8_t status, bool considerErasedAsValid)
{
	bool rtnVal = false;

	if (address >= ((uint32_t) _sectorsReservedForFAT + (uint32_t) _sectorsReservedForErase) * 4096 && address < _capacity) {
		if (address + fileSize < _capacity) {
			if (status == 0xff || status == 0x00) {
				rtnVal = true;
			}
		}
	}

	if (considerErasedAsValid) {
		//*** Empty entries can be valid, in their own way
		if (address == 0xffffffff && fileSize == 0xffffffff && status == 0xff) {
			rtnVal = true;
		}
	}

	return rtnVal;
}
