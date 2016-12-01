#include "SPI.h"
#include "SPIFlash.h"
#include "SPIFlashFS.h"


/****

Status List

001  uninitialized
100  ready
101  accepting data
200  error erasing chip
201  error intializing filesystem
202  error getting file size
203  invalid file size
204  error creating file
205  error writing to file

****/



SPIFlash flash;
SPIFlashFS* fs;
uint8_t  status;


//*** union used for byte array to int converstion
union
{
	byte b[sizeof(int32_t)];
	int32_t I;
} int32union;


//*** function declarations
bool copyDataToFile(SPIFlashFSFile*, uint32_t);
bool eraseChip();




void
setup()
{
	Serial.begin(115200);

	flash.begin();

	status = 1;  //*** uninitialized
}



void
loop()
{
	SPIFlashFSFile* file;
	uint32_t filesize;
	int input = Serial.read();


	switch (input) {
		//***  "s" - get status
		case 115:
			Serial.write(status);
			break;


		//***  "e" - erase the chip
		case 101:
			if (eraseChip()) {
				status = 1;  //*** uninitialized
			} else {
				status = 200;  //*** error erasing chip
			}

			Serial.write(status);
			break;


		//***  "i" - initialize the filesystem
		case 105:

			if (fs) {
				delete fs;
			}

			fs = new SPIFlashFS(&flash);

			if (fs->isInitialized()) {
				status = 100;  //*** ready
			} else {
				status = 201;  //*** error intializing filesystem
			}

			Serial.write(status);
			break;


		//***  "f" - create new file
		case 102:
			Serial.write(status);

			if (status == 100) {
				for (uint16_t i = 0; i < (sizeof(int32union.I)); i++) {
					do {
						input = Serial.read();
					} while (input == -1);

					int32union.b[i] = input;
				}

				filesize = int32union.I;

				if (filesize > 0) {

					file = fs->createFile(filesize);
					if (file) {

						status = 101;  //*** accepting data

						if (!copyDataToFile(file, filesize)) {
							status = 205;
						} else {
							status = 100;  //*** ready
						}

						//*** The result from SPIFlashFS.createFile will be an object created with "new"
						//*** When finished using said file we must delete it.
						delete file;

					} else {
						status = 204;  //*** error creating file
					}

				} else {
					status = 203;  //*** invalid file size
				}

				Serial.write(status);
			}
			break;
	} 
}



bool
copyDataToFile(SPIFlashFSFile* file, uint32_t filesize)
{
	bool rtnVal = true;
	uint8_t buffer[256];
	uint16_t bytesWritten;
	uint16_t bytesToWrite, bytesToRead;
	uint32_t totalBytesWritten = 0;

	while (totalBytesWritten < filesize) {

		Serial.write(status);

		bytesToRead = min(256, filesize - totalBytesWritten);
		bytesToWrite = Serial.readBytes(buffer, bytesToRead);

		if (bytesToWrite > 0) {
			bytesWritten = file->writeByteArray(buffer, bytesToWrite);
			totalBytesWritten += bytesWritten;

			if (bytesWritten != bytesToWrite) {
				rtnVal = false;
				break;
			}
		} else {
			rtnVal = false;
			break;
		}
	}

	return rtnVal;
}


bool
eraseChip()
{
	bool rtnVal = false;

	if (flash.eraseChip()) {

		while (flash.readByte(0) != 255) {
			delay(1500);
		}

		rtnVal = true;
	}

	return rtnVal;
}
