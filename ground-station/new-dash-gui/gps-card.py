import multiprocessing
from pprint import pprint
from multiprocessing.managers import BaseManager
import sys
import time
import datetime
import random
import pdb
from queue import Empty
import dash
import dash_leaflet as dl
import dash_bootstrap_components as dbc
from dash import dcc, html, Input, Output, callback, State
import plotly.graph_objs as go

import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS


class DummyRocket:
    """Creates objects that simulates the velocity and altitude of a rocket when launched.

    The rocket simulation is very rough but follows that of
    a generic parabolic curve given the time (x-value) the rocket should apogee.
    The ``duration`` parameter of ``launch()`` should be changed to account for any change in
    the burnout_time and apogee_time, but it isn't necessary.

    TODO: Add more attributes to the rocket that would be transmitted down to the ground station
    """

    def __init__(self):
        self.time_series_data = {"Time": [], "Positions": [[37.7749, -122.4194],],}  # start in San Fran for pos

    # Need to make getter and setter funcs bc proxy objs are spawned by the manager,
    #   so direct access is not possible
    def set(self, min_dict):
        for key, value in min_dict.items():
            self.time_series_data[key].append(value)
        # print(self.time_series_data)

    def get(self):
        return self.time_series_data
#################Time stamp 6:43 2/10/25
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
         

def dashboard(rocket):
    external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

    app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

    # deep copy of rocket, pass by value
    rocketo_proxy = rocket
    data = rocket.get()

    # Initial positions
    positions = data["Positions"]
    #print(f"line 65, last pos: {positions[-1]}")

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
        )]


        
    )

    # boundaries for the simulated path (coordinates within the USA)
    

    

    @app.callback(
        
        Output("path", "positions"),
        Input("interval-component", "n_intervals"),
        State("path", "positions"),
    )
    def update_map_live(n, positions):
        

        #current_positions = data["Positions"]
        new_position = rocketo_proxy.generate_new_position()
        
        #print(f"in update_graph_live: newest pos: {new_position}")
        data = rocketo_proxy.get()
        return data["Positions"]
        # return positions

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

        data = rocketo_proxy.get()
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


    app.run_server(debug=True, port=8060)

def add_values(rocket):
    

    # pprint(vars(rocket))
    # simulate receiving new coordinates
    data = rocket.get()
    current_positions = data["Positions"]
    print("Add values just ran")
    
    return current_positions


def populate_infludb(rocket):
    token = "b1IZkUyPFfN1-WOtxgQN8eYmpup1c61MZbioelVzqe7bQ3pIZgmxFDyly8WtOdqEL7LeG9o30Hs9lf-IrhozQA=="
    org = "TuftsRocketry"
    bucket = "bucket4"
    url = "http://localhost:8086/"

    client = InfluxDBClient(url=url, token=token)
    write_api = client.write_api(write_options=SYNCHRONOUS)

    rocketo_proxy = rocket
    data = rocketo_proxy.get()

    last_index = -1

    def write_data_points(time, altitude, velocity, last_index):
        points = []
        for i in range(last_index + 1, len(time)):
            point = (
                Point("launch_data")
                .tag("launch", "Spaceport")
                .field("altitude", altitude[i])
                .field("velocity", velocity[i])
                .field("rocket time", time[i])
                .time(datetime.datetime.now())
            )
            points.append(point)
            # print(int(time[i]), altitude[i], velocity[i])
            last_index = i
        if points:
            write_api.write(bucket=bucket, org=org, record=points)
        return last_index

    def close_client():
        print("Closing client...")
        client.close()
        sys.exit(0)

    try:
        while True:
            last_index = write_data_points(
                data["Time"], data["Altitude"], data["Velocity"], last_index
            )

    except KeyboardInterrupt:
        print("Interrupted by User")
        close_client()


if __name__ == "__main__":
    BaseManager.register("DummyRocket", DummyRocket)
    manager = BaseManager()
    manager.start()
    inst = manager.DummyRocket()

    p1 = multiprocessing.Process(target=add_values, args=[inst])
    p2 = multiprocessing.Process(target=dashboard, args=[inst])
    #p3 = multiprocessing.Process(target=populate_infludb, args=[inst])

    p1.start()
    time.sleep(3)
    p2.start()
    #p3.start()

    p1.join()
    p2.join()
    #p3.join()