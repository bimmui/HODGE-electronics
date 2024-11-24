import serial
import time

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	time.sleep(10)
	serialController.write(str.encode("Howdy\n", "utf-8"))
	timeSent = time.time()
	print(serialController.readline())
	timeRecieved = time.time()
	print(timeRecieved - timeSent)