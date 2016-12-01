#include <SPI.h>
#include <SPIFlash.h>
#include <SPIFlashFS.h>
#include <SD.h>



SPIFlash flash;



bool eraseChip = false;



void setup() 
{
	Serial.begin(115200);
	Serial.println("Initializing...");
	Serial.println();

	flash.begin();


	if (eraseChip) {

		Serial.println("Erasing the SPIFlash chip");
		Serial.println();


		if (flash.eraseChip()) {
			Serial.print("Success");

			while (flash.readByte(0) != 255) {
				Serial.print(".");
				delay(1000);
			}

			Serial.println();

		} else {
			Serial.println("Failure");
			return;
		}
	}


	SPIFlashFS fs(&flash);


	if (fs.isInitialized()) {

		SPIFlashFSFile* spiFile;
		uint16_t fileSize;

		File root = SD.open("/");


		//  loop through sd card files here
		while (true) {

			File sdcFile = root.openNextFile();

			if (!sdcFile) {
				break;
			}

			if (sdcFile.isDirectory()) {
				Serial.print("Directory ");
				Serial.print(sdcFile.name());
				Serial.println(" skipped.  Only files in the root are copied.");

				continue;
			}

			fileSize = sdcFile.size();

			spiFile = fs.createFile(fileSize);

			if (spiFile) {
				uint16_t bytesWritten = 0;

				while (sdcFile.available()) {
					if (!spiFile->writeByte(sdcFile.read())) {
						break;
					}
					bytesWritten++;
				}

				if (bytesWritten == fileSize) {
					Serial.print(sdcFile.name());
					Serial.print(" successfully written to memory.  It was assigned file number ");
					Serial.print(spiFile->getFileNum());
					Serial.print(" and contains ");
					Serial.print(fileSize);
					Serial.println(" bytes of data.");
				} else {
					Serial.print(sdcFile.name());
					Serial.print(" failed to write to memory.  ");
					Serial.print(bytesWritten);
					Serial.println(" bytes were written before the failure.");
				}

				//*** The result from SPIFlashFS.openFile will be an object created with "new"
				//*** When finished using said file we must delete it.
				delete spiFile;

			} else {

				Serial.print("Unable to create ");
				Serial.println(sdcFile.name());
			}
		}
	}
}



void loop() {
  // nothing happens after setup
}


