#Dependency Imports
import multiprocessing.managers
import time
import serial
import multiprocessing
import os
import dotenv
import dash
from dash import dcc, html, Input, Output, callback, State
import dash_bootstrap_components as dbc
import dash_leaflet as dl
import plotly.graph_objs as go

import random

import cProfile
import re

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
		
		self.time_series_data = {
            "Time": [],
            "Positions": [[37.7749, -122.4194],], # start in San Fran for pos
            "Internal Temp": [],
            "External Temp": [],
            "Acceleration": [],
            "Velocity": []
		}
		self.start_time = time.time()
		
	#Destructor: kills processes when object is garbage collected
	#Makes sure processes are killed on dereference
	def __del__(self):
		self.kill()


	# Need to make getter and setter funcs bc proxy objs are spawned by the manager,
    #   so direct access is not possible
	def set(self, min_dict):
		for key, value in min_dict.items():
			self.time_series_data[key].append(value)
        # print(self.time_series_data)

	def get(self):
		return self.time_series_data
	
	def generate_new_position(self):
		last_position = self.time_series_data["Positions"][-1]
		min_lat, max_lat = 24.396308, 49.384358  # min and Max latitude for the continental US
		min_lon, max_lon = -125.0, -66.93457  # min and Max longitude for the continental US
		new_lat = last_position[0] + random.uniform(-0.5, 0.5)
		new_lon = last_position[1] + random.uniform(-0.5, 0.5)
		new_lat = min(max(new_lat, min_lat), max_lat)
		new_lon = min(max(new_lon, min_lon), max_lon)
		self.time_series_data["Positions"].append([new_lat, new_lon])

		return [new_lat, new_lon]
    
	def generate_new_temp(self):

		elapsed_time = time.time() - self.start_time

		if elapsed_time <= 15:
			internal_temp = 13 + elapsed_time*5
			external_temp = 13 - elapsed_time*5
		else:
			time_since_burnout = elapsed_time - 5
			internal_temp = 13 + 5*15 - .5 * time_since_burnout
			external_temp = 13 - 5*15 + 3 * time_since_burnout
				
		new_data = {
			"Time": elapsed_time,
			"Internal Temp": internal_temp,
			"External Temp": external_temp,
			
		}
		self.set(new_data)
		#return internal_temp, external_temp
		return 0
    
	def generate_new_accel_and_vel(self):

		elapsed_time = time.time() - self.start_time
		time_since_burnout = elapsed_time - 5

		if elapsed_time <= 15:
			accel = 2 * elapsed_time
			velocity = 6.5 * elapsed_time
			
		else:
			accel = -9.8
			velocity = 6.5 * 15 - 2 * time_since_burnout
				
		new_data = {
			"Time": elapsed_time,
			"Acceleration": accel,
			"Velocity" : velocity
			
		}
		self.set(new_data)

		return 0

	#This process reads data from the shared memory class and logs it to the DB
	#shared_memory_reference (Manager): a reference to the shared memory where daata can be pulled from.  The Manager should have registered a SharedMemory class
	#token (string): the DB token
	#org (string): the DB organization name
	#bucket (string): the DB bucket which SerialReader should write to
	#table_name (string): the name of the DB table which the SerialReader should write to (influxDB calls this table a "measurement")
	#field_names (array: string): An array containing names of the DB fields which the SerialReader should write to
	def _log_shared_memory_to_database(self, shared_memory_reference, token, org, url, bucket, table_name, field_names):
		#cProfile.run('re.compile("foo|bar")')
		print("Dummy")
	#This process reads data from the serial input and logs it to the shared memory class
	#When processing data from the serial connection, a comma (",") delineates a separation of values, while a newline ("\n") delineates a separation of entries
	#shared_memory_reference (Manager): a reference to the shared memory where daata can be pulled from.  The Manager should have registered a SharedMemory class
	#serial_connection_path (string): the path that represents the USB connection.  For Linux, it comes in the form: "/dev/ttyACM*" where "*" is a number
	#baud_rate (int): the baud rate of the serial connection. It should match the baud rate specified in the arduino code.  
	def _read_serial(self, shared_memory_reference, serial_connection_path, baud_rate):
		altitude = 0
		velocity = 20
		flight_time = 0
		for i in range(10000):
			shared_memory_reference.write([flight_time, altitude, velocity])
			altitude += 1
			velocity -= 1
			flight_time += 1
			time.sleep(0.01)
	
	def dashboard(self, shared_memory_reference):
		external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

		app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

		# deep copy of rocket, pass by value
		
		data = self.get()

		# Initial positions
		positions = data["Positions"]

		app.layout = html.Div(
			[html.Div(
				[
					html.H4("Rocket Launch Simulation"),
					dl.Map(
						[
							dl.TileLayer(),
							dl.FullScreenControl(),
							dl.Polyline(id="path", positions=positions),

						],
						center=[37.7749, -122.4194],
						zoom=5,
						style={"height": "50vh"},
						id="map",
					),
					
					dcc.Interval(
						id="interval-component",
						interval=1 * 1000,  # in milliseconds
						n_intervals=0,
					),
				]
			),

			dbc.Card(
				dbc.CardBody(
					[
						html.H4("GPS", className="card-title"),
						html.P(
							[
								html.Span(
									"Coordinates: ",
									style={"fontWeight": "bold", "textDecoration": "underline"},
								),
								html.Span(id="coordinates", className="card-text"),
							]
						),
						html.P(
							[
								html.Span(
									"Altitude (m): ",
									style={"fontWeight": "bold", "textDecoration": "underline"},
								),
								html.Span(id="altitude-m", className="card-text"),
							]
						),
						html.P(
							[
								html.Span(
									"Altitude (ft): ",
									style={"fontWeight": "bold", "textDecoration": "underline"},
								),
								html.Span(id="altitude-ft", className="card-text"),
							]
						),
						html.P(
							[
								html.Span(
									"Signal Quality: ",
									style={"fontWeight": "bold", "textDecoration": "underline"},
								),
								html.Span(id="signal_quality", className="card-text"),
							]
						),
						html.P(
							[
								html.Span(
									"GPS Fix: ",
									style={"fontWeight": "bold", "textDecoration": "underline"},
								),
								html.Span(id="gps_fix", className="card-text"),
							]
						),
						html.P(
							[
								html.Span(
									"Antenna Status: ",
									style={"fontWeight": "bold", "textDecoration": "underline"},
								),
								html.Span(id="antenna_status", className="card-text"),
							]
						),
					]
				)
			),
			html.Div([
				dcc.Graph(id="live-update-temp-graph"),
				dcc.Graph(id = "live-update-alt-graph"),
				dcc.Graph(id="live-update-accel-gauge"),
				dcc.Graph(id="live-update-vel-gauge"),
			])
    	]
        
    	)
		
		@app.callback(
			Output("path", "positions"),
			Input("interval-component", "n_intervals"),
			State("path", "positions"),
		)

		def update_map_live(n, positions):
			

			#current_positions = data["Positions"]
			new_position = self.generate_new_position()
			
			#print(f"in update_graph_live: newest pos: {new_position}")
			data = self.get()
			return data["Positions"]
			# return positions


		@app.callback(
			Output("live-update-temp-graph", "figure"),
			Input("interval-component", "n_intervals"),
		)
		
		def update_temp_graph_live(n):
			self.generate_new_temp()
			data = self.get()

			fig = go.Figure()
			fig.add_trace(
				go.Scatter(
					x=data["Time"],
                	y=data["Internal Temp"],
					mode="lines+markers",
					name="Internal Temp (°C)",
					hoverinfo="none",
				)
			)
			fig.add_trace(
				go.Scatter(
					x=data["Time"],
					y=data["External Temp"],
					mode="lines+markers",
					name="External Temp (°C)",
					hoverinfo="none",
				)
			)

			fig.update_layout(
				title_text="Temperatures",
				xaxis_title="Time",
				yaxis_title="Value",
				title_x=0.5,
				height=600,
				showlegend=True,
				hovermode="closest",
			)

			return fig
		


		@app.callback(
			[
				Output("coordinates", "children"),
				Output("altitude-m", "children"),
				Output("altitude-ft", "children"),
				Output("signal_quality", "children"),
				Output("gps_fix", "children"),
				Output("antenna_status", "children"),
			],
			Input("interval-component", "n_intervals"),
				
		)

		def update_gps_status(n):
		# Simulate data update, replace with actual data retrieval

			data = self.get()
			current_positions = data["Positions"]
			#print(f"latest pos: {current_positions[-1]}")


			new_values = {
				"coordinates": f"Lat: {current_positions[-1][0]:.2f}, Lon: {current_positions[-1][1]:.2f}",
				"altitude-m": f"{random.uniform(0, 10000):.2f} m",
				"altitude-ft": f"{random.uniform(0, 10000):.2f} ft",
				"signal_quality": f"{random.randint(0, 100)}",
				"gps_fix": "Yes" if random.choice([True, False]) else "No",
				"antenna_status": (
					"Connected" if random.choice([True, False]) else "Disconnected"
				),
			}

			return (
				f" {new_values['coordinates']}",
				f" {new_values['altitude-m']}",
				f" {new_values['altitude-ft']}",
				f" {new_values['signal_quality']}",
				f" {new_values['gps_fix']}",
				f" {new_values['antenna_status']}",
			)
		
		@app.callback(
			[
				Output("live-update-accel-gauge", "figure"),
				Output("live-update-vel-gauge", "figure"),
			],
			
			Input("interval-component", "n_intervals"),
		)

		def update_accel_gauge_live(n):
			
			
			self.generate_new_accel_and_vel()
			
			data = self.get()
			accelFig = go.Figure(
				go.Indicator(
					mode="gauge+number",
					value=0,
					
					gauge={
						"shape": "angular",
						"axis": {"range": [-10, 40]},
						"bar": {"color": "#FFA07A"},  # Use black to hide the bar
						"steps" : [
							{'range': [-10, -9.8], 'color': 'red'},
							{'range': [-9.8, 0], 'color': 'yellow'},
							{'range': [0, 40], 'color': 'green'},
						],
						"bgcolor": "#FFA07A",
						"threshold": {
							"line": {"color": "#800020", "width": 6},
							"thickness": 0.75,
							"value": 100,
						},
					},
					domain={"x": [0, 1], "y": [0, 1]},
				)
			)
			gaugeLine = data["Velocity"][-1]

			velFig = go.Figure(
				go.Indicator(
					mode="gauge+number",
					value=gaugeLine,
					
					gauge={
						"shape": "angular",
						"axis": {"range": [-30, 100]},
						"bar": {"color": "rgba(0,0,0,0)"},  # Use black to hide the bar
						"steps" : [
							{'range': [-30, 0], 'color': 'lightgray'},
							{'range': [0, 100], 'color': 'green'}
						],
						"bgcolor": "#FFA07A",
						"threshold": {
							"line": {"color": "#cef2ef", "width": 6},
							"thickness": 0.75,
							"value": gaugeLine,
						},
					},
					
					domain={"x": [0, 1], "y": [0, 1]},
				)
			)
			accelFig.update_traces(value=data["Acceleration"][-1])
			velFig.update_traces(value=data["Velocity"][-1])
			#return data["Acceleration"]
			# return positions

			return accelFig, velFig


#-----------------------------------------
		@app.callback(
			Output("live-update-alt-graph", "figure"),
			Input("interval-component", "n_intervals"),
		)
		def update_alt_graph_live(n):

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

		app.run(debug=True, port=8060, host="0.0.0.0")
	#Starts the both processes in the SerialReader class
	def start(self):

		#If _process_1 is defined and is currently running, then do no reinstatniate it
		if not self._process_1.is_alive():
			self._process_1.start()

		if not self._process_2.is_alive():
			self._process_2.start()

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
token = "b1IZkUyPFfN1-WOtxgQN8eYmpup1c61MZbioelVzqe7bQ3pIZgmxFDyly8WtOdqEL7LeG9o30Hs9lf-IrhozQA==" #os.environ["DB_TOKEN"]
org = "TuftsRocketry" #os.environ["DB_ORG"]
#token = os.environ["DB_TOKEN"]
#org = os.environ["DB_ORG"]

"""
#Load environment variables
dotenv.load_dotenv()
token = "b1IZkUyPFfN1-WOtxgQN8eYmpup1c61MZbioelVzqe7bQ3pIZgmxFDyly8WtOdqEL7LeG9o30Hs9lf-IrhozQA==" #os.environ["DB_TOKEN"]
org = "TuftsRocketry" #os.environ["DB_ORG"]

#Not sensitive info
url = "http://localhost:8086" #uncomment this value for local testing
#url = "http://192.168.1.181:8086" #uncomment this value if doing remote testing

#bucket = "Test"
bucket = "bucket4"
# token = "b1IZkUyPFfN1-WOtxgQN8eYmpup1c61MZbioelVzqe7bQ3pIZgmxFDyly8WtOdqEL7LeG9o30Hs9lf-IrhozQA=="
#     org = "TuftsRocketry"
#     bucket = "bucket4"
"""

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
	print(f"PID: {os.getpid()}")
	my_serial_reader.start()
	my_serial_reader.join_processes()