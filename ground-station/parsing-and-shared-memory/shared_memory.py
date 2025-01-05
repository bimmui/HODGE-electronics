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

		#Gets data in a node
		def getData(self):
			return self.data
		
		#Gets previous node
		def getPrev(self):
			return self.prev
		
		#Gets next node
		def getNext (self):
			return self.next

	#Constructor: initializes a linked list with a specified maximum length
	#maxLength: the maximum length of the linked list
	#token: the InfluxDB access token
	#org: the InfluxDB organization
	#url: the InfluxDB connection url (generally on port 8086)
	#bucket: the InfluxDB bucket you are writing to
	#tableName: the name of the InfluxDB measurement you are writing to
	#columnNames: the names of the fields in the InfluxDB measurement
	def __init__(self, maxLength=None, columnNames=None):
		self.maxLength = maxLength
		self.length = 1
		self.first = self.node(None, None, columnNames)
		self.last = self.first

		#self.database = DBHandler(token, org, url, bucket, tableName, columnNames)

	#Gets first node in linkedList	
	def getFirst(self):
		return self.first
	
	#Gets last node in linkedList
	def getLast(self):
		return self.last

	#Adds a new node to the shared memory linkedList, if the linked list length exceeds maxLength, then remove the last node
	#data: an array with a row to be added to a CSV sheet
	def write(self, data):
		nodeToAdd = self.node(None, self.first, data)
		self.first.next = nodeToAdd
		self.first = nodeToAdd
		self.length += 1

		if self.length > self.maxLength: #Remove last node if list is too long
			self.last = self.last.next
			self.length -= 1