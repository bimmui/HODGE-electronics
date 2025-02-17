#Dependency Imports
import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

#The DBHandler class opens a connection to an active InfluxDB, and allows you to write to it
class DBHandler:

	#Contructor: opens a connection to an InfluxDB.
	#token (string): the influxDB access token
	#org (string): the influxDB organization
	#url (string): the access url of the influx DB
	#table_name (string): the name of the table (measurement) which the data should be recorded to
	#field_names (array: string): an array containing the names of the fields which data should be recorded to in the table
	def __init__(self, token, org, url, bucket, table_name, field_names):
		self.org = org
		
		write_client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)
		self.write_api = write_client.write_api(write_options=SYNCHRONOUS)
		
		self.bucket = bucket
		self.table_name = table_name
		self.field_names = field_names
	
	#writes data to influx DB database
	#FIELD NAMES MUST BE INSTANTIATED BEFOREHAND
	#data (array: any): an array containing the data to be recorded.  The first element of the array corresponds with the first field, etc.  
	def write_to_database(self, data):
		
		#Fields corresponding to values must be a dictionary (field is the key, value is the value).  Therefore, we must rearrange data accordingly
		field_value_pairs= {}
		for i in range(len(self.field_names)):
			field_value_pairs[self.field_names[i]] = data[i]

		self.write_api.write(self.bucket, self.org, [{"measurement": self.table_name, "tags": {}, "fields": field_value_pairs}])