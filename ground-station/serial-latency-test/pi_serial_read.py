import serial
import time

attemptsToRun = 10

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	for i in range(attemptsToRun):
		
		time.sleep(5)
		print(f"Attempt ${i + 1}")

		serialController.write(str.encode("Howdy\n", "utf-8"))
		timeSent = time.time()
		print(serialController.readline())
		timeRecieved = time.time()

		print(timeRecieved - timeSent)