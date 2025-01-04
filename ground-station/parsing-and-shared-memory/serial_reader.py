import shared_memory
import db_handler

import serial
import multiprocessing

defaultToken = "jkbT8S2dsoHKx_MaG2A8zoboDJF04mssk-F6-1Vt-GMaAuQzlyakxF0ONJ3HEsitjXqd0NrQN0vvJ8qPnZv6MQ=="
defaultOrg = "TuftsSEDSRocketry"
#defautlUrl = "http://localhost:8086" #uncomment this value for local testing
defaultUrl = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
defaultBucket = "Test"
tableName = "Fruit Test With Serial"
fieldNames = ["Favorite", "Least Favorite", "Mid"]

mem = shared_memory.SharedMemory(3, fieldNames)
db = db_handler.DBHandler(defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames, mem)

entryStart = "["
entryEnd = "]"
valueSeparator = ","

with serial.Serial('/dev/ttyACM0', 57600) as serialController:
	serialController.flush()
	while True:
		addInput = serialController.readline().decode("utf-8").strip()
		#print(addInput)
		mem.write(addInput.split())
		print(mem.first.data)
