import serial
import time

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	timeRecieved = time.time()
	while True:
		readSerial = serialController.readline()
		timeRecievedUpdated = time.time()
		print(str(readSerial))
		print(f"Delay time: ${timeRecievedUpdated - timeRecieved}")
		timeRecieved = timeRecievedUpdated