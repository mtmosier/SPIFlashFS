/*
 * PlayRandomFileOnInterruptLowPower
 *
 * Audio playing via interrupt is based upon speaker_pcm
 * http://playground.arduino.cc/Code/PCMAudio
 *
 *
 * Plays 8-bit PCM audio on pin 11 using pulse-width modulation (PWM).
 * For Arduino with Atmega168 at 16 MHz.
 *
 * Uses two timers. The first changes the sample value 8000 times a second.
 * The second holds pin 11 high for 0-255 ticks out of a 256-tick cycle,
 * depending on sample value. The second timer repeats 62500 times per second
 * (16000000 / 256), much faster than the playback rate (8000 Hz), so
 * it almost sounds halfway decent, just really quiet on a PC speaker.
 *
 * Takes over Timer 1 (16-bit) for the 8000 Hz timer. This breaks PWM
 * (analogWrite()) for Arduino pins 9 and 10. Takes Timer 2 (8-bit)
 * for the pulse width modulation, breaking PWM for pins 11 & 3.
 *
 * References:
 *     http://www.uchobby.com/index.php/2007/11/11/arduino-sound-part-1/
 *     http://www.atmel.com/dyn/resources/prod_documents/doc2542.pdf
 *     http://www.evilmadscientist.com/article.php/avrdac
 *     http://gonium.net/md/2006/12/27/i-will-think-before-i-code/
 *     http://fly.cc.fer.hr/GDM/articles/sndmus/speaker2.html
 *     http://www.gamedev.net/reference/articles/article442.asp
 *
 * Michael Smith <michael@hurts.ca>
 */


/*
 * Updated 2016-11-30
 * Use pin 3 instead of 11 for audio output
 * Read the data from a global file object, in this case SPIFlashFSFile
 * Fade in and fade out, to avoid clicks
 *
 * Michael T. Mosier <mtmosier@gmail.com>
 */

/*
 * The audio data needs to be unsigned, 8-bit, 8000 Hz.
 *
 * The audio data should be saved as files on the external flash chip.
 * See example file SaveFilesFromSerialConnection or CopyFilesFromSDCardToSpiFlash
 * in order to save the files.
 *
 * http://musicthing.blogspot.com/2005/05/tiny-music-makers-pt-4-mac-startup.html
 * mplayer -ao pcm macstartup.mp3
 * sox audiodump.wav -v 1.32 -c 1 -r 8000 -u -1 macstartup-8000.wav
 * sox macstartup-8000.wav macstartup-cut.wav trim 0 10000s
 *
 * (starfox) nb. under sox 12.18 (distributed in CentOS 5), i needed to run
 * the following command to convert my wave file to the appropriate format:
 * sox audiodump.wav -c 1 -r 8000 -u -b macstartup-8000.wav
 */





#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "SPI.h"
#include "SPIFlash.h"
#include "SPIFlashFS.h"
#include "LowPower.h"



#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif




//*** Interrupt related variables / constants
const byte interruptPin = 2;
const unsigned long debounceMillis = 30;
bool lastState = true;
volatile unsigned long checkStatusMillis = 0;


//*** SPIFlash related variables / constants
SPIFlash flash;
SPIFlashFS* fs;
SPIFlashFSFile* sampleFile = NULL;


//*** Audio related variables / constants
#define SAMPLE_RATE 8000
const uint8_t audioOutPin = 3;
const uint8_t ampPowerPin = 9;
volatile bool isPlaying = false;


//*** Audio fade in/out
volatile bool fadeInActive = false;
volatile bool fadeOutActive = false;
volatile uint8_t fadeSample = 0;
const uint16_t fadeOutNumberOfSamples = 5 * 8; // 5ms @ 8 samples per ms
const uint16_t fadeInNumberOfSample = 5 * 8;  // 5ms @ 8 samples per ms
volatile uint16_t fadeSampleCount = 0;


//*** Debug
unsigned long lastLoopDebugOutputTime = 0;


//*** Function declarations
void goToSleep();
void wakeUpFromSleep();
void checkInterrupt();
void playFile();
void fadeInPlayback();
void fadeOutPlayback();
void startPlayback();
void stopPlayback();



void setup() 
{
	//*** Debugging output setup
	Serial.begin(115200);

	if (Serial) {
		Serial.println("Beginning audio player");
		Serial.println();
	}


	//*** Audio player setup
	if (Serial) {
		Serial.println("Initializing audio amp");
	}

	randomSeed(analogRead(0));
	pinMode(ampPowerPin, OUTPUT);
	digitalWrite(ampPowerPin, LOW);
	pinMode(audioOutPin, OUTPUT);
	digitalWrite(audioOutPin, LOW);


	//*** Filesystem setup
	if (Serial) {
		Serial.println("Initializing external memory");
	}

	flash.begin();

	if (Serial) {
		Serial.println("Reading file system");
	}

	fs = new SPIFlashFS(&flash);

	if (!fs->isInitialized()) {

		if (Serial) {
			Serial.println("Failed to initialize the FileSystem object");
			Serial.println("Please make sure the chip is connected properly and has either been erased already or was previously used with SPIFlashFS");
			Serial.println();
		}

		while (true) {}
	}


	//*** Interrupt setup
	if (Serial) {
		Serial.println("Initializing interrupt");
	}

	pinMode(interruptPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin), checkInterrupt, CHANGE);
}


void
loop()
{
	unsigned long curMillis = millis();

	if (Serial) {
		if (curMillis % 2000 == 0 && lastLoopDebugOutputTime != curMillis) {
			Serial.println("Processing loop");
			lastLoopDebugOutputTime = curMillis;
		}
	}

	if (checkStatusMillis < curMillis) {

		bool state = (digitalRead(interruptPin) == HIGH);

		if (state != lastState) {
			lastState = state;

			if (Serial) {
				Serial.print("State change, new state is ");
				Serial.println(state ? "On" : "Off");
			}

			if (!isPlaying && state) {
				wakeUpFromSleep();  //*** Ensure everything is powered up and ready
				playFile();
			} else if (isPlaying && !state) {
				fadeOutPlayback();
			}
		}

		if (!isPlaying) {
			goToSleep();
		}
	}
}


//***********************
//*** SLEEP FUNCTIONS ***
//***********************

void
goToSleep()
{
	if (Serial) {
		Serial.println("Going to sleep");
	}

	flash.powerDown();
	delay(25);

	LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}


void
wakeUpFromSleep()
{
	flash.powerUp();
	delay(25);

	if (Serial) {
		Serial.println("Waking up");
	}
}


//****************************
//*** INTERRUPT MONITORING ***
//****************************

void
checkInterrupt()
{
	if (checkStatusMillis < millis()) {
		checkStatusMillis = millis() + debounceMillis;

		if (Serial) {
			Serial.println("Interrupt fired");
		}
	}
}



//*****************************************
//*** AUDIO PROCESSING FUNCTIONS FOLLOW ***
//*****************************************

void
skipWaveFileHeader()
{
	//*** TODO - Skip to 36, check for "data", if found, skip 4 additional bytes, and done. If not, check location 44 for "data", etc
	//*** Also check the first 4 bytes. They should equal "RIFF" in big-endian format. If missing, this may be a pure pcm stream.

	//*** Advance the file past the wave file header
	sampleFile->seek(44);
}


void
playFile()
{
	uint16_t fileNum;

	if (sampleFile) {
		delete sampleFile;
		sampleFile = NULL;
	}

	fileNum = random(fs->getFileCount());
	sampleFile = fs->openFile(fileNum);

	if (sampleFile && sampleFile->available()) {
		startPlayback();
	} else {
		if (Serial) {
			Serial.println("Unable to open file for reading");
		}
	}
}

void
fadeInPlayback()
{
	if (Serial) {
		Serial.println("Fade in");
	}

	skipWaveFileHeader();
	fadeSample = sampleFile->readByte();

	fadeInActive = true;
	fadeOutActive = false;
	fadeSampleCount = 0;
}


void
fadeOutPlayback()
{
	if (Serial) {
		Serial.println("Fade out");
	}

	//*** The last sound byte played is stored in OCR2B. We fade out off of that high
	fadeSample = OCR2B;
	fadeInActive = false;
	fadeOutActive = true;
	fadeSampleCount = 0;
}


void
startPlayback()
{
	// Set up Timer 2 to do pulse width modulation on the speaker
	// pin.

	// Use internal clock (datasheet p.160)
	ASSR &= ~(_BV(EXCLK) | _BV(AS2));

	// Set fast PWM mode  (p.157)
	TCCR2A |= _BV(WGM21) | _BV(WGM20);
	TCCR2B &= ~_BV(WGM22);

	// Do non-inverting PWM on pin OC2B (p.155)
	// On the Arduino this is pin 3.
	TCCR2A = (TCCR2A | _BV(COM2B1)) & ~_BV(COM2A0);
	TCCR2A &= ~(_BV(COM2A1) | _BV(COM2B0));

	// No prescaler (p.158)
	TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);

	// Set initial pulse width to 0, which will be faded in
	digitalWrite(ampPowerPin, HIGH);
	OCR2B = 0;
	isPlaying = true;

	// Set up Timer 1 to send a sample every interrupt.

	cli();

	// Set CTC mode (Clear Timer on Compare Match) (p.133)
	// Have to set OCR1A *after*, otherwise it gets reset to 0!
	TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
	TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));

	// No prescaler (p.134)
	TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);

	// Set the compare register (OCR1A).
	// OCR1A is a 16-bit register, so we have to do this with
	// interrupts disabled to be safe.
	OCR1A = F_CPU / SAMPLE_RATE;    // 16e6 / 8000 = 2000

	// Enable interrupt when TCNT1 == OCR1A (p.136)
	TIMSK1 |= _BV(OCIE1A);

	fadeInPlayback();

	sei();
}


void
stopPlayback()
{
	if (Serial) {
		Serial.println("Stop playback");
	}

	// Disable playback per-sample interrupt.
	TIMSK1 &= ~_BV(OCIE1A);

	// Disable the per-sample timer completely.
	TCCR1B &= ~_BV(CS10);

	// Disable the PWM timer.
	TCCR2B &= ~_BV(CS10);

	digitalWrite(ampPowerPin, LOW);
	digitalWrite(audioOutPin, LOW);

	isPlaying = false;
}


// This is called at 8000 Hz to load the next sample.
ISR(TIMER1_COMPA_vect)
{
	if (fadeOutActive) {
		if (fadeSampleCount < fadeOutNumberOfSamples) {
			OCR2B = fadeSample * (1 - ((float) fadeSampleCount / (float) fadeOutNumberOfSamples));
			fadeSampleCount++;
		} else {
			stopPlayback();
		}

	} else if (fadeInActive) {

		if (fadeSampleCount < fadeInNumberOfSample) {
			OCR2B = fadeSample * ((float) fadeSampleCount / (float) fadeInNumberOfSample);
			fadeSampleCount++;
		} else {
			OCR2B = fadeSample;
			fadeInActive = false;
		}

	} else {

		if (!sampleFile->available()) {
			fadeOutPlayback();
		} else {
			OCR2B = sampleFile->readByte();
		}
	}
}
