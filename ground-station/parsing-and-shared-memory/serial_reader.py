import multiprocessing.managers
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

mem = shared_memory.SharedMemory(100, fieldNames)
valueSeparator = ","
#\n is the entry separator

def logSharedMemoryToDB(sharedMemoryReferenceList, defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames):
	dbReference = db_handler.DBHandler(defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames)
	sharedMemoryReference = sharedMemoryReferenceList[0]
	while True:
		if sharedMemoryReferenceList[0].first.data != fieldNames:
			dbReference.writeToDB(sharedMemoryReferenceList[0].first.data)
			print("Logged!")
			#time.sleep(0.01) #Temporarily here for now

def readSerial(sharedMemoryReferenceList):
	sharedMemoryReference = sharedMemoryReferenceList[0]
	with serial.Serial('/dev/ttyACM0', 57600) as serialController:
		serialController.flush()
		while True:
			addInput = serialController.readline().decode("utf-8").strip()
			#print(addInput)
			sharedMemoryReference.write(addInput.split(valueSeparator))
			print(sharedMemoryReference.first.data)

class CustomManager(multiprocessing.managers.BaseManager):
	pass
CustomManager.register("list", list)

with multiprocessing.Manager() as manager:
	sharedMemList = manager.list([mem])
	p1 = multiprocessing.Process(target=logSharedMemoryToDB, args=(sharedMemList,))
	p2 = multiprocessing.Process(target=readSerial, args=(sharedMemList,defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames))

	p1.start()
	p2.start()

	p1.join()
	p2.join()

