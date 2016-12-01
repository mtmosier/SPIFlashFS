#include "SPI.h"
#include "SPIFlash.h"
#include "SPIFlashFS.h"


SPIFlash flash;


void setup() 
{
	uint16_t fileCount;
	uint32_t lastAddress = 0, lastSize = 0;
	uint32_t bytesWasted = 0;
	uint32_t bytesRemaining = 0;


	Serial.begin(115200);
	Serial.println("Beginning test");
	Serial.println();

	flash.begin();

	SPIFlashFS fs(&flash);


	if (fs.isInitialized()) {

		fileCount = fs.getFileCount();

		Serial.print(fileCount);
		Serial.println(" files to be checked...");
		Serial.println();

		for (uint8_t idx = 0; idx < fileCount; idx++) {
			uint32_t address, size;
			uint8_t  status;
			bool entryValid = true;

			fs.loadFileInfo(idx, &address, &size, &status);

			entryValid = fs.isFATEntrySane(address, size, status);
			if (!entryValid) {
				Serial.println("Inavlid file entry!");
				Serial.println();
			}

			Serial.print("File number: ");
			Serial.println(idx);
			Serial.print("File address: ");
			Serial.println(address);
			Serial.print("File size: ");
			Serial.println(size);
			Serial.print("File status: ");
			Serial.println(status);

			if (status == 0xff && entryValid) {
				SPIFlashFSFile* file;

				file = fs.openFile(idx);

				if (file) {
					uint8_t dataByte;
					uint8_t bytesToRead = min(32, size);

					Serial.println("File header:");

					for (uint8_t pos = 0; pos < bytesToRead; pos++) {
						file->seek(pos);
						dataByte = file->readByte();

						Serial.print(dataByte);
						if ((pos + 1) % 8) {
							Serial.print(' ');
						} else {
							Serial.println();
						}
					}

					if (bytesToRead % 8 != 0) {
						Serial.println();
					}

					if (lastAddress > 0) {
						bytesWasted += address - lastAddress - size;
					}

					lastAddress = address;
					lastSize = size;

					//*** The result from SPIFlashFS.openFile will be an object created with "new"
					//*** When finished using said file we must delete it.
					delete file;

				} else {
					Serial.println("Unable to open file for reading");
				}
			}

			Serial.println();
			Serial.println();

			bytesRemaining = fs.getCapacity() - lastAddress - lastSize;
		}

		Serial.print(bytesWasted);
		Serial.println(" wasted bytes were found between file entries.");

		Serial.print(bytesRemaining);
		Serial.println(" bytes available.");


	} else {

		Serial.println("Failed to initialize the FileSystem object");
		Serial.println("Please make sure the chip is connected properly and has either been erased already or was previously used with SPIFlashFS");
		Serial.println();
	}
}



void loop() {
  // nothing happens after setup
}


