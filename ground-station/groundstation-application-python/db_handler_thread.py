from __future__ import annotations

#from typing import TypeVar, Generic, Optional
from influxdb_client.client.influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS, WriteApi

from thread_handler import ThreadHandler
from shared_memory import SharedMemory

# The DBHandlerThread class opens a connection to an influxDB database and contains a thread to continually log data to the database
class DBHandlerThread (ThreadHandler):
	
	# Constructor: opens a connection to the influxDB database
	# token (str): the influxDB access token
	# org (str): the influxDB organization name
	# bucket (str): the name of the bucket the data should be recorded to
	# table_name (str): the name of the table (measurement) which the data should be recorded to
	def __init__(self, memory_manager: SharedMemory, url: str, token: str, org: str, bucket: str, table_name: str):
		super().__init__(memory_manager, True)

		self._org: str = org
		self._bucket: str = bucket
		self._table_name: str = table_name
		self._field_names: list[str] = self.memory_manager.get_column_names()
		
		write_client: InfluxDBClient = InfluxDBClient(url=url, token=token, org=org, debug=False)
		self._api_writer: WriteApi = write_client.write_api(SYNCHRONOUS)
		
	#writes data to influx DB database
	#data (list[any]): an array containing the data to be recorded.  The first element of the array corresponds with the first field, etc.  
	def write_to_database(self, data: list[float]):
		
		#Fields corresponding to values must be a dictionary (field is the key, value is the value).  Therefore, we must rearrange data accordingly
		if len(self._field_names) != len(data):
			print("DB data does not fit field names")
			return
		field_value_pairs: dict[str, float] = {}

		for i in range(len(self._field_names)):
			field_value_pairs[self._field_names[i]] = data[i]

		self._api_writer.write(self._bucket, self._org, [{"measurement": self._table_name, "tags": {}, "fields": field_value_pairs}])

	def thread_function(self):
		if self.memory_manager.get_first() != None:
			self.write_to_database(self.memory_manager.get_first().get_data())
