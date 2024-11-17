import serial

with serial.Serial('/dev/ttyACM0', 9600) as serialController:
	while True:
		readSerial = serialController.readline();
		print(str(readSerial))