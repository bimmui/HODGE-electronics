import serial
import time

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	serialController.write(to_bytes("Howdy"))
	timeSent = time.time()
	print(serialController.readline())
	timeRecieved = time.time()
	print(timeRecieved - timeSent)