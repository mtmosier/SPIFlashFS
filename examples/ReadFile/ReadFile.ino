#include "SPI.h"
#include "SPIFlash.h"
#include "SPIFlashFS.h"


SPIFlash flash;



void setup() 
{
	Serial.begin(115200);
	Serial.println("Beginning test");
	Serial.println();

	flash.begin();
	SPIFlashFS fs(&flash);

	if (fs.isInitialized()) {

		SPIFlashFSFile* file;

		file = fs.openFile(fs.getFileCount() - 1);

		if (file) {
			Serial.println("File contents:");
			Serial.println();

			for (uint32_t pos = 1; pos <= file->getFileSize(); pos++) {
				uint8_t dataByte = file->readByte();

				if (dataByte < 16) {
					Serial.print(' ');
				}
				Serial.print(dataByte, HEX);
				if (pos % 8) {
					Serial.print(' ');
				} else {
					Serial.println();
				}
			}

			Serial.println();
			Serial.println();

			//*** The result from SPIFlashFS.openFile will be an object created with "new"
			//*** When finished using said file we must delete it.
			delete file;

		} else {
			Serial.println("Unable to open file for reading");
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


