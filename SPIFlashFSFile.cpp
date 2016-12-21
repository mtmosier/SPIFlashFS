#include "SPIFlashFSFile.h"


const bool performFastRead = false;



SPIFlashFSFile::SPIFlashFSFile(SPIFlash *flashObj, SPIFlashFS *fsObj)
{
	_initialized = false;
	_flashObj = flashObj;
	_fsObj = fsObj;
	_currentPosition = 0;
}


SPIFlashFSFile::SPIFlashFSFile(SPIFlash *flashObj, SPIFlashFS *fsObj, uint16_t fileNum)
{
	_initialized = false;
	_flashObj = flashObj;
	_fsObj = fsObj;
	_currentPosition = 0;

	loadFile(fileNum);
}


bool
SPIFlashFSFile::isInitialized()
{
	return _initialized;
}


bool
SPIFlashFSFile::loadFile(uint16_t fileNum)
{
	bool success;
	bool rtnVal = false;
	uint32_t address = 0;
	uint32_t fileSize = 0;


	success = _fsObj->loadFileInfo(fileNum, &address, &fileSize);

	if (success && fileSize > 0) {
		_fileNum = fileNum;
		_fileSize = fileSize;
		_fileLocationAddress = address;
		_currentPosition = 0;
		_initialized = true;

		rtnVal = true;
	}

	return rtnVal;
}


uint32_t
SPIFlashFSFile::initNewFile(uint32_t fileSize)
{
	bool success;
	uint32_t rtnVal = 0;
	uint32_t address = 0;
	uint16_t fileNum = 0xffff;


	success = _fsObj->allocateNewFile(fileSize, &address, &fileNum);

	if (success && fileNum != 0xffff) {
		_fileNum = fileNum;
		_fileSize = fileSize;
		_fileLocationAddress = address;
		_currentPosition = 0;
		_initialized = true;

		rtnVal = fileNum;
	}

	return rtnVal;
}


bool
SPIFlashFSFile::writeByte(uint8_t dataByte)
{
	bool result = false;
	uint32_t address = 0;

	if (isInitialized() && available()) {
		address = _getCurrentAddress();
		result = _flashObj->writeByte(address, dataByte);

		if (result) {
			//*** Increment position
			seek(getFilePosition() + 1);
		}
	}

	return result;
}


uint16_t
SPIFlashFSFile::writeByteArray(uint8_t *dataBuffer, uint16_t bufferSize)
{
	uint16_t bytesToWrite = bufferSize;
	uint32_t address = 0;
	uint32_t curFilePosition;

	if (isInitialized() && available()) {

		curFilePosition = getFilePosition();

		if (curFilePosition + bytesToWrite >= _fileSize) {
			bytesToWrite = _fileSize - curFilePosition;
		}

		address = _getCurrentAddress();
		_flashObj->writeByteArray(address, dataBuffer, bytesToWrite);

		//*** Adjust position
		seek(getFilePosition() + bytesToWrite);
	}

	return bytesToWrite;
}


uint8_t
SPIFlashFSFile::readByte()
{
	uint8_t dataByte = 0;
	uint32_t address = 0;

	if (isInitialized() && available()) {

		address = _getCurrentAddress();
		dataByte = _flashObj->readByte(address, performFastRead);

		//*** Increment position
		seek(getFilePosition() + 1);
	}

	return dataByte;
}


uint16_t
SPIFlashFSFile::readByteArray(uint8_t *dataBuffer, uint16_t bufferSize)
{
	uint16_t bytesToRead = 0;
	uint32_t address = 0;

	if (isInitialized() && available()) {

		bytesToRead = bufferSize;

		if (getFilePosition() + bytesToRead >= _fileSize) {
			bytesToRead = _fileSize - getFilePosition();
		}

		address = _getCurrentAddress();
		_flashObj->readByteArray(address, dataBuffer, bytesToRead, performFastRead);

		//*** Adjust position
		seek(getFilePosition() + bytesToRead);
	}

	return bytesToRead;
}



bool
SPIFlashFSFile::available()
{
	if (getFilePosition() < _fileSize) {
		return true;
	}
	return false;
}


bool
SPIFlashFSFile::eraseFile()
{
	bool eraseSuccessful = false;
	uint32_t address = 0;

	if (isInitialized()) {
		uint16_t sectorsToDelete = (uint16_t) floor((_fileSize - 1) / 4096) + 1;

		eraseSuccessful = true;
		for (uint16_t i = 0; i < sectorsToDelete; i++) {
			seek(4096 * i);
			address = _getCurrentAddress();

		    if (_flashObj->eraseSector(address)) {
				while (_flashObj->readByte(address) != 255)  delay(250);
			} else {
				eraseSuccessful = false;
			}
		}

		_fsObj->removeFileEntry(_fileNum);

		_initialized = false;
	}

	return eraseSuccessful;
}


bool
SPIFlashFSFile::seek(uint32_t position)
{
	if (!isInitialized())  return false;

	//*** Bounds checking
	if (position >= _fileSize) {
		_currentPosition = _fileSize;
	} else {
		_currentPosition = position;
	}

	return true;
}


uint32_t
SPIFlashFSFile::getFilePosition()
{
	return _currentPosition;
}


uint16_t
SPIFlashFSFile::getFileNum()
{
	return _fileNum;
}


uint32_t
SPIFlashFSFile::getFileSize()
{
	return _fileSize;
}


uint32_t
SPIFlashFSFile::_getCurrentAddress()
{
	if (!isInitialized())  return;
	return getFilePosition() + _fileLocationAddress;
}
