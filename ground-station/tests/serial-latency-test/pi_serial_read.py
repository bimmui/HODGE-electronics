import serial
import time

attemptsToRun = 10
delayTimes = []

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	time.sleep(5)
	for i in range(attemptsToRun):
		
		print(f"Attempt {i + 1}:")
		time.sleep(1)

		serialController.write(str.encode("Howdy\n", "utf-8"))
		timeSent = time.time()
		print(serialController.readline())
		timeRecieved = time.time()

		delayTimes.append(timeRecieved - timeSent)
		print(f"Delay time (s): {delayTimes[i]}\n")
	
	timeSum = 0
	for i in range(len(delayTimes)):
		timeSum += delayTimes[i]
	print(f"Average delay time {timeSum / i} seconds")
