import random
import dash
from dash import dcc, html
from dash.dependencies import Input, Output
import dash_bootstrap_components as dbc
import plotly.graph_objects as go
import dash_daq as daq

app = dash.Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP])


# Function to create a custom gauge with a label
def create_gauge(gauge_id, label_text, value=5):
    fig = go.Figure(
        go.Indicator(
            mode="gauge+number",
            value=value,
            gauge={
                "shape": "angular",
                "axis": {"range": [0, 10]},
                "bar": {"color": "#FFA07A"},  # Use black to hide the bar
                "bgcolor": "#FFA07A",
                "threshold": {
                    "line": {"color": "#800020", "width": 6},
                    "thickness": 0.75,
                    "value": value,
                },
            },
            domain={"x": [0, 1], "y": [0, 1]},
        )
    )

    fig.update_layout(
        margin=dict(l=20, r=20, t=50, b=20),
        height=200,
    )

    return html.Div(
        [
            dcc.Graph(id=gauge_id, figure=fig, config={"staticPlot": True}),
            html.P(label_text, style={"text-align": "center", "margin-top": "5px"}),
        ]
    )


# Define the layout of the dashboard
app.layout = dbc.Container(
    [
        html.H1("Custom Gauge Example"),
        dbc.Row(
            [
                dbc.Col(
                    [
                        html.Div("Custom Gauge with Plotly"),
                        create_gauge("custom-gauge1", "Speed"),
                        create_gauge("custom-gauge2", "Temperature"),
                    ]
                ),
                dbc.Col(
                    [
                        html.Div("Custom Gauge with Plotly"),
                        create_gauge("custom-gauge3", "Speed"),
                        create_gauge("custom-gauge4", "Temperature"),
                    ]
                ),
                dcc.Interval(
                    id="interval-component",
                    interval=200,  # Update every 200 milliseconds
                    n_intervals=0,
                ),
                dbc.Col(
                    [
                        html.Div("Custom Gauge with Plotly"),
                        create_gauge("custom-gauge5", "Speed"),
                        create_gauge("custom-gauge6", "Temperature"),
                    ]
                ),
                dcc.Interval(
                    id="interval-component",
                    interval=200,  # Update every 200 milliseconds
                    n_intervals=0,
                ),
            ]
        ),
    ],
    fluid=True,
)


# Create a callback to update the gauges
@app.callback(
    [
        Output("custom-gauge1", "figure"),
        Output("custom-gauge2", "figure"),
        Output("custom-gauge3", "figure"),
        Output("custom-gauge4", "figure"),
        Output("custom-gauge5", "figure"),
        Output("custom-gauge6", "figure"),
    ],
    [Input("interval-component", "n_intervals")],
)
def update_gauges(n):
    # Simulate data update, replace with actual data retrieval
    new_values = [random.uniform(0, 10) for _ in range(6)]
    gauges = []
    for value in new_values:
        fig = go.Figure(
            go.Indicator(
                mode="gauge+number",
                value=value,
                gauge={
                    "shape": "angular",
                    "axis": {"range": [0, 10]},
                    "bar": {"color": "#FFA07A"},  # Use black to hide the bar
                    "bgcolor": "#FFA07A",
                    "threshold": {
                        "line": {"color": "#800020", "width": 4},
                        "thickness": 0.75,
                        "value": value,
                    },
                },
                domain={"x": [0, 1], "y": [0, 1]},
            )
        )

        fig.update_layout(
            margin=dict(l=20, r=20, t=50, b=20),
            height=200,
        )
        gauges.append(fig)

    return gauges


if __name__ == "__main__":
    app.run_server(debug=True)
