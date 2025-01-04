import shared_memory as SM
import serial

defaultToken = "jkbT8S2dsoHKx_MaG2A8zoboDJF04mssk-F6-1Vt-GMaAuQzlyakxF0ONJ3HEsitjXqd0NrQN0vvJ8qPnZv6MQ=="
defaultOrg = "TuftsSEDSRocketry"
#defautlUrl = "http://localhost:8086" #uncomment this value for local testing
defaultUrl = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
defaultBucket = "Test"
tableName = "Fruit Test With Serial"
fieldNames = ["Favorite", "Least Favorite", "Mid"]

mem = SM.SharedMemory(3, defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames)

entryStart = "["
entryEnd = "]"
valueSeparator = ","

with serial.Serial('/dev/ttyACM0', 57600) as serialController:
	inputString = ""
	while True:
		addInput = serialController.read(1000)
		inputString = addInput + inputString
		if(addInput.find(entryStart)):
			inputToWrite = addInput[addInput.find(entryStart):addInput.find(entryEnd)]
			print(inputToWrite)
