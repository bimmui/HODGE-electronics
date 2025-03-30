from __future__ import annotations
from typing import Union

import random

import dash
import dash_leaflet as dl
import dash_bootstrap_components as dbc
from dash import dcc, html, Input, Output, callback, State
import plotly.graph_objs as go

from shared_memory import SharedMemory
from thread_handler import ThreadHandler

#The DashboardThread class runs the dashboard in a thread.  
class DashboardNewThread (ThreadHandler):
	
	#Constructor: assigns the memory_manager of the thread, and the port and url of the Dash application
	#memory_manager (SharedMemory): the shared memory the thread can pull from
	#port (int): the port of the dash application
	#address (str): the url (IP address) of the dash application
	def __init__(self, memory_manager: SharedMemory, port: int = 8060, address: str = "0.0.0.0"):
		super().__init__(memory_manager, False)
		
		self._port: int = port
		self._address: str = address

	#Opens a Dash application at the 
	#Overrides ThreadHandler's abstract function
	def thread_function(self):
		external_stylesheets: list[Union[str, dict[str, object]]] = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

		app: dash.Dash = dash.Dash(__name__, external_stylesheets=external_stylesheets)

        
		app.layout = html.Div(
        [html.Div(
            [
                html.H4("Rocket Launch Simulation"),
                dl.Map(
                    [
                        dl.TileLayer(),
                        dl.FullScreenControl(),
                        dl.Polyline(id="path", positions=self.memory_manager.convert_to_array(4),),

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
		
		html.Div(
            [
                html.H4("Rocket Launch Simulation"),
                dcc.Graph(id="live-update-graph"),
                dcc.Interval(
                    id="interval-component",
                    interval= 0.1 * 1000,  # in milliseconds
                    n_intervals=0,
                ),
            ]
        ),
			
        html.Div(
			[
                dcc.Graph(id="live-update-temp-graph"),
                dcc.Graph(id="live-update-accel-gauge"),
                dcc.Graph(id="live-update-vel-gauge"),
            ]
		)
        ])
		

		@app.callback(
			Output("path", "positions"),
			Input("interval-component", "n_intervals"),
            State("path", "positions"),
        )
		def update_map_live(n, positions, self):
            #current_positions = data["Positions"]
            #new_position = rocketo_proxy.generate_new_position()
            
            #print(f"in update_graph_live: newest pos: {new_position}")
            #data = rocketo_proxy.get()
            #return positions
			return self.memory_manager.convert_to_array(4)
		

		@app.callback(
            Output("live-update-temp-graph", "figure"),
            Input("interval-component", "n_intervals"),
		)

		def update_temp_graph_live(n):

			fig = go.Figure()
			fig.add_trace(
                go.Scatter(
                    x=self.memory_manager.convert_to_array(0),
                    y=self.memory_manager.convert_to_array(5),
                    mode="lines+markers",
                    name="Internal Temp (°C)",
                    hoverinfo="none",
                )
            )
			fig.add_trace(
                go.Scatter(
                    x=self.memory_manager.convert_to_array(0),
                    y=self.memory_manager.convert_to_array(6),
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
			current_positions = self.memory_manager.convert_to_array(4)
			new_values = {
                "coordinates": f"Lat: {current_positions[-1][0]:.2f}, Lon: {current_positions[-1][1]:.2f}",
                "altitude-m": f"{self.memory_manager.convert_to_array(1):.2f} m",
                "altitude-ft": f"{(self.memory_manager.convert_to_array(1))*3.28:.2f} ft",
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
                Output("live-update-graph", "figure"),
                Input("interval-component", "n_intervals"),
        )
		
        
		def update_graph_live(n):
			fig: go.Figure = go.Figure()
			fig.add_trace(
				go.Scatter(
					x=self.memory_manager.convert_to_array(0),
					y=self.memory_manager.convert_to_array(1),
					mode="lines+markers",
					name="Altitude",
					hoverinfo="none",
				)
			)
			fig.add_trace(
				go.Scatter(
					x=self.memory_manager.convert_to_array(0),
					y=self.memory_manager.convert_to_array(2),
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
		
		@app.callback(
            [
                Output("live-update-accel-gauge", "figure"),
                Output("live-update-vel-gauge", "figure"),
            ],
            
            Input("interval-component", "n_intervals"),
        )
		
		def update_accel_gauge_live(n):
			
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
			gaugeLine = self.memory_manager.convert_to_array(2)[-1]
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
			accelFig.update_traces(value=self.memory_manager.convert_to_array(3)[-1])
			velFig.update_traces(value=self.memory_manager.convert_to_array(2)[-1])
			
			return accelFig, velFig

		app.run(debug=True, port = str(self._port), host = self._address)