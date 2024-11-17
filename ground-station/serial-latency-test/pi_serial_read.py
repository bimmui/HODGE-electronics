import serial
import time

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	serialController.write(str.encode("Howdy", "utf-8"))
	timeSent = time.time()
	print(serialController.read(1))
	timeRecieved = time.time()
	print(timeRecieved - timeSent)