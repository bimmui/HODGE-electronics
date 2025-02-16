#Dependency Imports
import multiprocessing.managers
import time
import serial
import multiprocessing
import os
import dotenv
import dash
from dash import dcc, html, Input, Output, callback
import plotly.graph_objs as go

#Class imports
import shared_memory
import db_handler

#The SerialReader runs two processes:
# - Reads data from a serial connection, parses it, and places it logs it to shared memory.  
# - Continually logs the first data from shared memory into the influxDB
#ONLY TO BE USED IN TOP LEVEL CODE
#This is being implemented as a class because we can initialize two objects for each antenna
class ProcessHandler():

	#Constructor: initializes a SerialReader class, which must be started by calling the start() function. 
	#ENCAPSULATE WITH if __name__ = "__main__" 
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

		#_shared_mem is a member to prevent it being dereferenced (and garbage collected).  This would cause an error
		self._shared_memory_object = manager.SharedMemory(shared_memory_list_length, field_names)
		self.field_names = field_names

		#create both processes
		self._process_1 = multiprocessing.Process(target=self._log_shared_memory_to_database, args=(self._shared_memory_object, token, org, url, bucket, table_name, field_names))
		self._process_2 = multiprocessing.Process(target=self._read_serial, args=(self._shared_memory_object, serial_connection_path, baud_rate))
		self._process_3 = multiprocessing.Process(target=self.dashboard, args=(self._shared_memory_object,))
		
	#Destructor: kills processes when object is garbage collected
	#Makes sure processes are killed on dereference
	def __del__(self):
		self.kill()

	#This process reads data from the shared memory class and logs it to the DB
	#shared_memory_reference (Manager): a reference to the shared memory where daata can be pulled from.  The Manager should have registered a SharedMemory class
	#token (string): the DB token
	#org (string): the DB organization name
	#bucket (string): the DB bucket which SerialReader should write to
	#table_name (string): the name of the DB table which the SerialReader should write to (influxDB calls this table a "measurement")
	#field_names (array: string): An array containing names of the DB fields which the SerialReader should write to
	def _log_shared_memory_to_database(self, shared_memory_reference, token, org, url, bucket, table_name, field_names):
		while True:
			continue
	#This process reads data from the serial input and logs it to the shared memory class
	#When processing data from the serial connection, a comma (",") delineates a separation of values, while a newline ("\n") delineates a separation of entries
	#shared_memory_reference (Manager): a reference to the shared memory where daata can be pulled from.  The Manager should have registered a SharedMemory class
	#serial_connection_path (string): the path that represents the USB connection.  For Linux, it comes in the form: "/dev/ttyACM*" where "*" is a number
	#baud_rate (int): the baud rate of the serial connection. It should match the baud rate specified in the arduino code.  
	def _read_serial(self, shared_memory_reference, serial_connection_path, baud_rate):
		altitude = 0
		velocity = 20
		flight_time = 0
		for i in range(20):
			shared_memory_reference.write([flight_time, altitude, velocity])
			altitude += 1
			velocity -= 1
			flight_time += 1
			time.sleep(1)
	
	def dashboard(self, shared_memory_reference):
		external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

		app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

		app.layout = html.Div(
			html.Div(
				[
					html.H4("Rocket Launch Simulation"),
					dcc.Graph(id="live-update-graph"),
					dcc.Interval(
						id="interval-component",
						interval=1 * 1000,  # in milliseconds
						n_intervals=0,
					),
				]
			)
		)

		@app.callback(
			Output("live-update-graph", "figure"),
			Input("interval-component", "n_intervals"),
		)
		def update_graph_live(n):

			fig = go.Figure()
			fig.add_trace(
				go.Scatter(
					x=shared_memory_reference.convert_to_array(0),
					y=shared_memory_reference.convert_to_array(1),
					mode="lines+markers",
					name="Altitude",
					hoverinfo="none",
				)
			)
			fig.add_trace(
				go.Scatter(
					x=shared_memory_reference.convert_to_array(0),
					y=shared_memory_reference.convert_to_array(2),
					mode="lines+markers",
					name="Velocity",
					hoverinfo="none",
				)
			)

			fig.update_layout(
				title_text="Rocket Altitude and Velocity",
				xaxis_title="Time",
				yaxis_title="Value",
				title_x=0.5,
				height=600,
				showlegend=True,
				hovermode="closest",
			)

			return fig

		app.run_server(debug=True, port=8060)
	#Starts the both processes in the SerialReader class
	def start(self):

		#If _process_1 is defined and is currently running, then do no reinstatniate it
		if not self._process_1.is_alive():
			self._process_1.start()

		#if not self._process_1.is_alive():
			#self._process_1.start()

		if not self._process_3.is_alive():
			self._process_3.start()

	#Wait for processes to terminate (never happens currently: processes run until keyboard interrupt)
	def join_processes(self):
		self._process_1.join()
		self._process_2.join()
		self._process_3.join()
	
	#kill both processes in SerialReader
	def kill(self):
		self._process_1.kill()
		self._process_2.kill()
		self._process_3.kill()

	#Returns the SharedMemory linked list 
	#Return: (SharedMemory)
	def get_shared_memory(self):
		return self._shared_memory_object

#Load environment variables
dotenv.load_dotenv()
token = os.environ["DB_TOKEN"]
org = os.environ["DB_ORG"]

#Not sensitive info
url = "http://localhost:8086" #uncomment this value for local testing
#url = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
bucket = "Test"
table_name = "Fruit Test With Serial 2"
field_names = ["Time", "Altitude", "Velocity"]
serial_connection_path = "/dev/ttyACM0"
baud_rate = 115200
shared_memory_length = 1000

if __name__ == "__main__":
	my_serial_reader = ProcessHandler(token, org, url, bucket, table_name, field_names, serial_connection_path, baud_rate, shared_memory_length)
	my_serial_reader.start()
	my_serial_reader.join_processes()