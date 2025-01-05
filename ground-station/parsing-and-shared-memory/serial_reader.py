import multiprocessing.managers
import shared_memory
import db_handler

import serial
import multiprocessing
import time

defaultToken = "jkbT8S2dsoHKx_MaG2A8zoboDJF04mssk-F6-1Vt-GMaAuQzlyakxF0ONJ3HEsitjXqd0NrQN0vvJ8qPnZv6MQ=="
defaultOrg = "TuftsSEDSRocketry"
#defautlUrl = "http://localhost:8086" #uncomment this value for local testing
defaultUrl = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
defaultBucket = "Test"
tableName = "Fruit Test With Serial 2"
fieldNames = ["Favorite", "Least Favorite", "Mid"]

valueSeparator = ","
#\n is the entry separator

#This process reads data from the shared memory class and logs it to the DB
def logSharedMemoryToDB(sharedMemoryReferenceList, token, org, url, bucket, tableName, fieldNames):
	dbReference = db_handler.DBHandler(token, org, url, bucket, tableName, fieldNames)
	while True:
		if sharedMemoryReferenceList.getFirst().getData() != fieldNames:
			dbReference.writeToDB(sharedMemoryReferenceList.getFirst().getData())
			print("Logged!")
			#time.sleep(0.01) #Temporarily here for now

#This process reads data from the serial input and logs it to the shared memory class
def readSerial(sharedMemoryReferenceList):
	with serial.Serial('/dev/ttyACM0', 57600) as serialController:
		serialController.flush()
		while True:
			addInput = serialController.readline().decode("utf-8").strip()
			#print(addInput)
			sharedMemoryReferenceList.write(addInput.split(valueSeparator))
			print(sharedMemoryReferenceList.getFirst().getData())

#class CustomManager(multiprocessing.managers.BaseManager):
#	pass
#CustomManager.register("list", list)

manager = multiprocessing.managers.BaseManager()
manager.register("SharedMemory", shared_memory.SharedMemory)
manager.start()

sharedMem = manager.SharedMemory(100, fieldNames)

p1 = multiprocessing.Process(target=logSharedMemoryToDB, args=(sharedMem, defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames))
p2 = multiprocessing.Process(target=readSerial, args=(sharedMem,))

p1.start()
p2.start()

p1.join()
p2.join()

