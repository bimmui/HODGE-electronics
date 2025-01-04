import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

#This class opens a connection to an active InfluxDB, and allows you to write to it
class DBHandler:

	#Contructor: opens a link to an InfluxDB.
	#token: the influxDB access token
	#org: the influxDB organization
	#url: the access url of the influx DB
	#tableName: the name of the table (measurement) which the data should be recorded to
	#fieldNames: an array containing the names of the fields which data should be recorded to in the table
	#sharedMemoryReference: a reference to a sharedMemory class
	def __init__(self, token, org, url, bucket, tableName, fieldNames, sharedMemoryReference):
		self.org = org
		self.writeClient = influxdb_client.InfluxDBClient(url=url, token=token, org=org)
		self.writeApi = self.writeClient.write_api(write_options=SYNCHRONOUS)
		
		self.bucket = bucket
		self.tableName = tableName
		self.fieldNames = fieldNames

		self.sharedMemoryReference = sharedMemoryReference
	
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

	#writes last data from sharedMemory class to influx DB
	def writeLastFromSharedMemory(self):
		self.writeToDB(self.sharedMemoryReference.last.data)

SM.write(["apple", "banana", "broccoli"])
SM.write(["pineapple", "turnip", "Swiss chard"])
SM.write(["carrot", "mango", "Dragonfruit"])
print(SM.last.data)
print(SM.first.data)