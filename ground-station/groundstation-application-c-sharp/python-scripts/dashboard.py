import dash
from dash import dcc, html, Input, Output, callback
import plotly.graph_objs as go

display_data_array = []

def update_graph_data(data):
	display_data_array = data

external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

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
def update_graph_live(n):

	fig = go.Figure()
	fig.add_trace(
		go.Scatter(
			x=display_data_array[0],
			y=display_data_array[1],
			mode="lines+markers",
			name="Altitude",
			hoverinfo="none",
		)
	)
	fig.add_trace(
		go.Scatter(
			x=display_data_array[0],
			y=display_data_array[1],
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