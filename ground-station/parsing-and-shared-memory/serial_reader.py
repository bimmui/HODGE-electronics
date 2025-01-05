import shared_memory
import db_handler

import serial
import multiprocessing

defaultToken = "jkbT8S2dsoHKx_MaG2A8zoboDJF04mssk-F6-1Vt-GMaAuQzlyakxF0ONJ3HEsitjXqd0NrQN0vvJ8qPnZv6MQ=="
defaultOrg = "TuftsSEDSRocketry"
#defautlUrl = "http://localhost:8086" #uncomment this value for local testing
defaultUrl = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
defaultBucket = "Test"
tableName = "Fruit Test With Serial 2"
fieldNames = ["Favorite", "Least Favorite", "Mid"]

mem = shared_memory.SharedMemory(3, fieldNames)
db = db_handler.DBHandler(defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames)

valueSeparator = ","
#\n is the entry separator

processStarted = False


def logSharedMemoryToDB(sharedMemoryReferenceList, dbReference):
	sharedMemoryReference = sharedMemoryReferenceList[0]
	while True:
		if sharedMemoryReference.first.data != fieldNames:
			dbReference.writeToDB(mem.first.data)
			print("Logged!")
			#time.sleep(0.01) #Temporarily here for now

def readSerial(sharedMemoryReferenceList):
	sharedMemoryReference = sharedMemoryReferenceList
	with serial.Serial('/dev/ttyACM0', 57600) as serialController:
		serialController.flush()
		while True:
			addInput = serialController.readline().decode("utf-8").strip()
			#print(addInput)
			sharedMemoryReference.write(addInput.split(valueSeparator))
			print(sharedMemoryReference.first.data)

with multiprocessing.Manager() as manager:
	sharedMemList = manager.list(mem, 0)
	p1 = multiprocessing.Process(target=logSharedMemoryToDB, args=(sharedMemList,db))
	p2 = multiprocessing.Process(target=readSerial, args=(sharedMemList,))

	p1.start()
	p2.start()

	p1.join()
	p2.join()

