from dash import Dash, dcc, html, State
import dash_leaflet as dl
from dash.dependencies import Input, Output
import random

app = Dash(__name__)

# Initial positions
positions = [[37.7749, -122.4194]]  # start in San Fran

app.layout = html.Div(
    [
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
        dcc.Interval(id="interval-component", interval=2000, n_intervals=0),
    ]
)

# boundaries for the simulated path (coordinates within the USA)
min_lat, max_lat = 24.396308, 49.384358  # min and Max latitude for the continental US
min_lon, max_lon = -125.0, -66.93457  # min and Max longitude for the continental US


def generate_new_position(last_position):
    new_lat = last_position[0] + random.uniform(-0.5, 0.5)
    new_lon = last_position[1] + random.uniform(-0.5, 0.5)
    new_lat = min(max(new_lat, min_lat), max_lat)
    new_lon = min(max(new_lon, min_lon), max_lon)
    return [new_lat, new_lon]


@app.callback(
    Output("path", "positions"),
    Input("interval-component", "n_intervals"),
    State("path", "positions"),
)
def update_path(n, current_positions):
    # simulate receiving new coordinates
    new_position = generate_new_position(current_positions[-1])
    current_positions.append(new_position)
    return current_positions


if __name__ == "__main__":
    app.run_server(debug=True)
