import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

#MAKE SURE TO START THE INFLUX DB BEFORE LAUNCHING

#The SharedMemory class is a linked list with that keeps track of data transferred from the arduino
#SharedMemory has a fixed length that is specified to prevent memory overflow
#SharedMemory opens a connection to an InfluxDB to write the data for long term use
class SharedMemory:

	#The node class represents a node in the linked the linked list
	class node:

		#Constructor: initializes a node in the linked list
		#next: the next node in the linked list
		#previous: the previous node in the linked list
		#data: the data stored by the node
		def __init__(self, next, previous, data):
			self.next = next
			self.prev = previous
			self.data = data

	#Constructor: initializes a linked list with a specified maximum length
	#Opens a connection to an InfluxDB as well
	#maxLength: the maximum length of the linked list
	#token: the InfluxDB access token
	#org: the InfluxDB organization
	#url: the InfluxDB connection url (generally on port 8086)
	#bucket: the InfluxDB bucket you are writing to
	#tableName: the name of the InfluxDB measurement you are writing to
	#columnNames: the names of the fields in the InfluxDB measurement
	def __init__(self, maxLength, token, org, url, bucket, tableName, columnNames):
		self.maxLength = maxLength
		self.length = 1
		self.first = self.node(None, None, columnNames)
		self.last = self.first

		self.database = DBHandler(token, org, url, bucket, tableName, columnNames)
		

	#Adds a new node to the shared memory linkedList, if the linked list length exceeds maxLength, then remove the last node
	#ONLY MEANT TO BE USED PRIVATELY
	#data: an array with a row to be added to a CSV sheet
	def _addNode(self, data):
		nodeToAdd = self.node(None, self.first, data)
		self.first.next = nodeToAdd
		self.first = nodeToAdd
		self.length += 1

		if self.length > self.maxLength: #Remove last node if list is too long
			self.last = self.last.next
			self.length -= 1

	#Adds a new node to the shared memory linkedList, if the linked list length exceeds maxLength, then remove the last node
	#Writes the data to the influx DB
	#data: an array with a row to be added to a CSV sheet
	def write(self, data):
		self._addNode(data)
		self.database.writeToDB(self.first.data) #Identical to self.database.writeToDB(data)

#This class opens a connection to an active InfluxDB, and allows you to write to it
class DBHandler:

	#Contructor: opens a link to an InfluxDB.  Fieldnames can be left empty on initialization but it must be specified before writing to the database
	#token: the influxDB access token
	#org: the influxDB organization
	#url: the access url of the influx DB
	#tableName: the name of the table (measurement) which the data should be recorded to
	#fieldNames: an array containing the names of the fields which data should be recorded to in the table
	def __init__(self, token, org, url, bucket, tableName, fieldNames=None):
		self.org = org
		self.writeClient = influxdb_client.InfluxDBClient(url=url, token=token, org=org)
		self.writeApi = self.writeClient.write_api(write_options=SYNCHRONOUS)

		self.bucket = bucket
		self.tableName = tableName
		self.fieldNames = fieldNames
	
	#sets the field names for data to be written to
	#fieldNames: an array containing the names of the fields which data should be recorded to in the table
	def setFieldNames(self, fieldNames):
		self.fieldNames = fieldNames

	#writes data to influx DB database
	#FIELD NAMES MUST BE INSTANTIATED BEFOREHAND
	#data: an array containing the data to be recorded.  The first element of the array corresponds with the first field, etc.  
	def writeToDB(self, data):

		#If field names are no instantiated, raise an error
		if self.fieldNames == None:
			raise Exception("Field Names not initialized")
		
		#Fields corresponding to values must be a dictionary (field is the key, value is the value).  Therefore, we must rearrange data accordingly
		fieldValuePairs= {}
		for i in range(len(self.fieldNames)):
			fieldValuePairs[self.fieldNames[i]] = data[i]

		self.writeApi.write(self.bucket, self.org, [{"measurement": self.tableName, "tags": {}, "fields": fieldValuePairs}])

defaultToken = "jkbT8S2dsoHKx_MaG2A8zoboDJF04mssk-F6-1Vt-GMaAuQzlyakxF0ONJ3HEsitjXqd0NrQN0vvJ8qPnZv6MQ=="
defaultOrg = "TuftsSEDSRocketry"
#defautlUrl = "http://localhost:8086" #uncomment this value for local testing
defaultUrl = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
defaultBucket = "Test"
tableName = "Fruit Test 3"
fieldNames = ["Favorite", "Least Favorite", "Mid"]

SM = SharedMemory(3, defaultToken, defaultOrg, defaultUrl, defaultBucket, tableName, fieldNames)
SM.write(["apple", "banana", "broccoli"])
SM.write(["pineapple", "turnip", "Swiss chard"])
SM.write(["carrot", "mango", "Dragonfruit"])
print(SM.last.data)
print(SM.first.data)