#Dependency Imports
import serial
import multiprocessing
import os
import dotenv

#Class imports
import shared_memory
import db_handler

#The SerialReader runs two processes:
# - Reads data from a serial connection, parses it, and places it logs it to shared memory.  
# - Continually logs the first data from shared memory into the influxDB
#
#This is being implemented as a class because we can initialize two objects for each antenna
class SerialReader():

	#Constructor: initializes a SerialReader class, which must be started by calling the start() function.  
	#token (string): the DB token
	#org (string): the DB organization name
	#bucket (string): the DB bucket which SerialReader should write to
	#table_name (string): the name of the DB table which the SerialReader should write to (influxDB calls this table a "measurement")
	#field_names (array: string): An array containing names of the DB fields which the SerialReader should write to
	#serial_connection_path (string): the path that represents the USB connection.  For Linux, it comes in the form: "/dev/ttyACM*" where "*" is a number
	#baud_rate (int): the baud rate of the serial connection. It should match the baud rate specified in the arduino code.  
	def __init__(self, token, org, url, bucket, table_name, field_names, serial_connection_path, baud_rate, shared_memory_list_length):
		#Create a manager, register the SharedMemory class with the manager, and start the manager
		manager = multiprocessing.managers.BaseManager()
		manager.register("SharedMemory", shared_memory.SharedMemory)
		manager.start()

		shared_mem = manager.SharedMemory(shared_memory_list_length, field_names)

		#create both processes
		self._process_1 = multiprocessing.Process(target=self._log_shared_memory_to_database, args=(shared_mem, token, org, url, bucket, table_name, field_names))
		self._process_2 = multiprocessing.Process(target=self._read_serial, args=(shared_mem, serial_connection_path, baud_rate))

	#This process reads data from the shared memory class and logs it to the DB
	#shared_memory_reference (Manager): a reference to the shared memory where daata can be pulled from.  The Manager should have registered a SharedMemory class
	#token (string): the DB token
	#org (string): the DB organization name
	#bucket (string): the DB bucket which SerialReader should write to
	#table_name (string): the name of the DB table which the SerialReader should write to (influxDB calls this table a "measurement")
	#field_names (array: string): An array containing names of the DB fields which the SerialReader should write to
	def _log_shared_memory_to_database(self, shared_memory_reference, token, org, url, bucket, table_name, field_names):
		db_reference = db_handler.DBHandler(token, org, url, bucket, table_name, field_names)
		while True:
			if shared_memory_reference.get_first().get_data() != field_names:
				db_reference.write_to_database(shared_memory_reference.get_first().get_data())
				print("Logged!")

	#This process reads data from the serial input and logs it to the shared memory class
	#When processing data from the serial connection, a comma (",") delineates a separation of values, while a newline ("\n") delineates a separation of entries
	#shared_memory_reference (Manager): a reference to the shared memory where daata can be pulled from.  The Manager should have registered a SharedMemory class
	#serial_connection_path (string): the path that represents the USB connection.  For Linux, it comes in the form: "/dev/ttyACM*" where "*" is a number
	#baud_rate (int): the baud rate of the serial connection. It should match the baud rate specified in the arduino code.  
	def _read_serial(self, shared_memory_reference, serial_connection_path, baud_rate):
		with serial.Serial(serial_connection_path, baud_rate) as serial_controller:
			serial_controller.flush() #whien initializing a serial connection, flush the buffer to get rid of unwanted input
			while True:
				add_input = serial_controller.readline().decode("utf-8").strip()
				shared_memory_reference.write(add_input.split(",")) #the split function formates the add_input into an array which we can log in the database
				print(shared_memory_reference.get_first().get_data())

	#Starts the both processes in the SerialReader class
	def start(self):

		#If _process_1 is defined and is currently running, then do no reinstatniate it
		if not self._process_1.is_alive():
			self._process_1.start()

		if not self._process_2.is_alive():
			self._process_2.start()

	#Wait for processes to terminate (never happens currently: processes run until keyboard interrupt)
	def join_processes(self):
		self._process_1.join()
		self._process_2.join()
	
	#kill both processes in SerialReader
	def kill(self):
		self._process_1.kill()
		self._process_2.kill()

#Load environment variables
dotenv.load_dotenv()
token = os.environ["DB_TOKEN"] #"jkbT8S2dsoHKx_MaG2A8zoboDJF04mssk-F6-1Vt-GMaAuQzlyakxF0ONJ3HEsitjXqd0NrQN0vvJ8qPnZv6MQ=="
org = os.environ["DB_ORG"] #"TuftsSEDSRocketry"

#Not sensitive info
#defautlUrl = "http://localhost:8086" #uncomment this value for local testing
url = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
bucket = "Test"
table_name = "Fruit Test With Serial 2"
field_names = ["Favorite", "Least Favorite", "Mid"]
serial_connection_path = "/dev/ttyACM0"
baud_rate = 57600
shared_memory_length = 1000

my_serial_reader = SerialReader(token, org, url, bucket, table_name, field_names, serial_connection_path, baud_rate, shared_memory_length)
my_serial_reader.start()
my_serial_reader.join_processes()