#include "SPI.h"
#include "SPIFlash.h"
#include "SPIFlashFS.h"


const uint8_t file1[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
const uint8_t file2[] = {
  20, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 0,
  21, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 1,
  22, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 2,
  23, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 3,
  24, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 4,
  25, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 5,
  26, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 6,
  27, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 7,
  28, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 8,
  29, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 9,
  30, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 10,
  31, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 11,
  32, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 12,
  33, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 13,
  34, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 14,
  35, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 15,
  36, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 16,
  37, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 17,
  38, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 18,
  39, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 19,
  40, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 20,
  41, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 21,
  42, 1, 18, 3, 16, 5, 14, 7, 12, 9, 10, 11, 8, 13, 6, 15, 4, 17, 2, 19, 22,
};
const uint8_t file3[] = {
  99, 88, 77, 66, 55, 44, 33, 22, 11, 0
};


bool eraseChip = false;



SPIFlash flash;


void setup() 
{
	Serial.begin(115200);
	Serial.println("Beginning test");
	Serial.println();

	flash.begin();


	if (eraseChip) {

		Serial.println("Erasing the SPIFlash chip");
		Serial.println();


		if (flash.eraseChip()) {
			Serial.print("Success");

			while (flash.readByte(0) != 255) {
				Serial.print(".");
				delay(500);
			}

			Serial.println();

		} else {
			Serial.println("Failure");
			return;
		}
	}


	SPIFlashFS fs(&flash);


	if (fs.isInitialized()) {

		SPIFlashFSFile* file;
		uint16_t fileSize;


		//*** Save file1
		fileSize = sizeof(file1);
		file = fs.createFile(fileSize);

		if (file) {

			uint16_t bytesWritten = file->writeByteArray(file1, fileSize);

			if (bytesWritten == fileSize) {
				Serial.print("File1 successfully written to memory.  It was assigned file number ");
				Serial.print(file->getFileNum());
				Serial.print(" and contains ");
				Serial.print(fileSize);
				Serial.println(" bytes of data.");
			} else {
				Serial.print("File1 failed to write to memory.  ");
				Serial.print(bytesWritten);
				Serial.println(" was returned by the write function.");
			}

			//*** The result from SPIFlashFS.openFile will be an object created with "new"
			//*** When finished using said file we must delete it.
			delete file;

		} else {

			Serial.println("Unable to create file1");
		}




		//*** Save file2
		fileSize = sizeof(file2);
		file = fs.createFile(fileSize);

		if (file) {

			uint16_t bytesWritten = 0;

			for (uint16_t i; i< fileSize; i++) {
				if (!file->writeByte(file2[i])) {
					break;
				}
				bytesWritten++;
			}

			if (bytesWritten == fileSize) {
				Serial.print("File2 successfully written to memory.  It was assigned file number ");
				Serial.print(file->getFileNum());
				Serial.print(" and contains ");
				Serial.print(fileSize);
				Serial.println(" bytes of data.");
			} else {
				Serial.print("File2 failed to write to memory.  ");
				Serial.print(bytesWritten);
				Serial.println(" was returned by the write function.");
			}

			//*** The result from SPIFlashFS.openFile will be an object created with "new"
			//*** When finished using said file we must delete it.
			delete file;

		} else {

			Serial.println("Unable to create file2");
		}




		//*** Save file3
		fileSize = sizeof(file3);
		file = fs.createFile(fileSize);

		if (file) {

			uint16_t bytesWritten = file->writeByteArray(file3, fileSize);

			if (bytesWritten == fileSize) {
				Serial.print("File3 successfully written to memory.  It was assigned file number ");
				Serial.print(file->getFileNum());
				Serial.print(" and contains ");
				Serial.print(fileSize);
				Serial.println(" bytes of data.");
			} else {
				Serial.print("File3 failed to write to memory.  ");
				Serial.print(bytesWritten);
				Serial.println(" was returned by the write function.");
			}

			//*** The result from SPIFlashFS.openFile will be an object created with "new"
			//*** When finished using said file we must delete it.
			delete file;

		} else {

			Serial.println("Unable to create file3");
		}

	} else {

		Serial.println("Failed to initialize the FileSystem object");
		Serial.println("Please make sure the chip is connected properly and has either been erased already or was previously used with SPIFlashFS");
		Serial.println();
	}
}



void loop() {
  // nothing happens after setup
}


