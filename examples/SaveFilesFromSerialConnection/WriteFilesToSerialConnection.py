import serial
from time import sleep
from glob import glob
import binascii
import os


inputDir = '/home/mtmosier/personal/projects/soundPlayer/8khz/'
inputPattern = inputDir + '*.wav'
device = '/dev/ttyUSB0'
baudrate = 115200






ser = serial.Serial(device, baudrate, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, 10)

sleep(2)


def sendByte(request):

	response = None

	print("Sending: " + request.decode("utf-8"));
	bytesWritten = ser.write(request)

	if bytesWritten > 0:
		response = ser.read(1)
		if response is not None:
			print("Received: " + str(response[0]))
		else:
			print("Timeout")
	else:
		print('Failed to write to serial connection.')

	return response


while True:
	response = sendByte(b'\x73');  # s - get status

	if response == b'\x01':  # 001 - uninitialized

		ser.timeout = 300  # the erase command takes a considerable amount of time
		response = sendByte(b'\x65')  # e - erase chip

		if response == b'\x01':  # 001 - uninitialized
			response = sendByte(b'\x69')  # i - initialize the filesystem


		ser.timeout = 10  # reset back to the normal

		if response != b'\x64':  # status other than 100 - ready means an error
			print("Unable to initialize the chip")
			break

	elif response == b'\x64':  # 100 - ready

		for filename in sorted(glob(inputPattern)):
			length = os.stat(filename).st_size

			print("Opening file " + filename + ' (' + str(length) + ')')
			inputFile = open(filename, 'rb')

			if inputFile:
				bytesRemaining = length
				bytesSent = 0

				response = sendByte(b'\x66');  # f - create new file

				if response == b'\x64': # 100 - ready

					print("Sending: " + str(length))
					bytesWritten = ser.write(length.to_bytes(4, 'little'))

					if bytesWritten != 4:
						print("Sent " + str(bytesWritten) + " bytes when intending to send 4")

					response = ser.read(1)
					if response is not None:
						if response is not None:
							print("Received: " + str(response[0]))
						else:
							print("Timeout")
					else:
						print("Timeout")

					if response == b'\x65': # 101 - accepting data
						while response == b'\x65': # 101 - accepting data

							bytesToRead = min(bytesRemaining, 256)

							data = inputFile.read(bytesToRead)

							if data:
								ser.write(data)

								bytesSent = bytesSent + len(data)
								bytesRemaining = length - bytesSent

								response = ser.read(1)
								if response is None:
									print("Timeout")

								if response != b'\x65' and response != b'\x64':
									print('Error received via the serial connection')

									if response is not None and length(response) > 0:
										print("Received: " + str(response[0]))

									break

							else:
								print('End of file reached')
								break

						if bytesRemaining < 1:
							print(str(bytesSent) + " bytes sent")
							print("Finished writing file")
						else:
							print("Incomplete file write. " + str(bytesRemaining) + " bytes not written")

					else:
						print('Invalid response trying to set a new file''s size')
						break

				else:
					print('Invalid response trying to create a new file')
					break

				inputFile.close()

			else:
				print('Failed opening file')
				break

		break

	else:
		print('Ready state not yet received.  Will retry.')
		sleep(1)


