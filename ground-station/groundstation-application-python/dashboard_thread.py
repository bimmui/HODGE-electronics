from __future__ import annotations
from typing import Union

import dash
from dash import dcc, html, Input, Output
import plotly.graph_objs as go

from shared_memory import SharedMemory
from thread_handler import ThreadHandler

#The DashboardThread class runs the dashboard in a thread.  
class DashboardThread (ThreadHandler):
	
	#Constructor: assigns the memory_manager of the thread, and the port and url of the Dash application
	#memory_manager (SharedMemory): the shared memory the thread can pull from
	#port (int): the port of the dash application
	#address (str): the url (IP address) of the dash application
	def __init__(self, memory_manager: SharedMemory, port: int = 8060, address: str = "0.0.0.0"):
		super().__init__(memory_manager, False)
		
		self._port: int = port
		self._address: str = address

	#Opens a Dash application at the 
	def thread_function(self):
		external_stylesheets: list[Union[str, dict[str, object]]] = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

		app: dash.Dash = dash.Dash(__name__, external_stylesheets=external_stylesheets)

		app.layout = html.Div(
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
			)
		)

		@app.callback(
			Output("live-update-graph", "figure"),
			Input("interval-component", "n_intervals"),
		)
		def update_graph_live(n) -> go.Figure:

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

		app.run(debug=True, port = str(self._port), host = self._address)