import serial
import time

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	serialController.write()
	timeSent = time.time();
	print(serialController.readline())
	timeRecieved = time.time()
	print(timeRecieved - timeSent)