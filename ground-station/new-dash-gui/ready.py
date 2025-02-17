import multiprocessing
from pprint import pprint
from multiprocessing.managers import BaseManager
import sys
import time
import datetime
import pdb
from queue import Empty
import dash
from dash import dcc, html, Input, Output, callback
import dash_daq as daq
import plotly.graph_objs as go
import random

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
        self.time_series_data = {"Time": [], "Velocity": []}

    # Need to make getter and setter funcs bc proxy objs are spawned by the manager,
    #   so direct access is not possible
    def set(self, min_dict):
        for key, value in min_dict.items():
            self.time_series_data[key].append(value)
        # print(self.time_series_data)

    def get(self):
        return self.time_series_data

    def get_latest(self, key):
        return self.time_series_data[key][-1]



def dashboard(rocket):
    external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

    app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

    app.layout = html.Div(
        html.Div(
            [
                html.H4("Rocket Launch Simulation"),
                daq.ToggleSwitch(
                    id='toggle-switch-1',
                    value=False,
                    size = 100,
                    # style = {'background-color': 'grey'},
                    theme = {'primary' : 'grey'},
                    color = 'grey',
                ),
                html.Div(id='toggle-switch-result-1'),
                # daq.ToggleSwitch(
                #     id='toggle-switch-2',
                #     value=False,
                #     size = 100,
                #     # style = {'background-color': 'grey'},
                #     theme = {'primary' : 'grey'},
                #     color = 'grey',
                # ),
                # html.Div(id='toggle-switch-result-2'),
                dcc.Interval(
                    id="interval-component",
                    interval=1 * 1000,  # in milliseconds
                    n_intervals=0,
                ),
            ]
        )
    )

    

    # deep copy of rocket, pass by value
    rocketo_proxy = rocket

    @app.callback(
        Output('toggle-switch-result-1', 'children'),
        Output('toggle-switch-1', 'color'),
        # Output('toggle-switch-result-2', 'children'),
        # Output('toggle-switch-2', 'color')],
        Input('toggle-switch-1', 'value'),
        # Input('toggle-switch-2', 'value2')],
    )
    def update_gauges(value):
        #print(f"Value 1: {value1}\nTest: {test}")
        data = rocketo_proxy.get()
        if value:
            return 'Switch is ON', 'green'
        else:
            return 'Switch is OFF', 'grey'
        #return f'Power {value}', {'background-color':color}

    app.run_server(debug=True, port=8060)


def add_values(rocket):
    def update_state(elapsed_time):
        if elapsed_time <= 15:
            velocity = 6.5 * elapsed_time
        else:
            time_since_burnout = elapsed_time - 5
            velocity = 6.5 * 15 - 2 * time_since_burnout


        return velocity

    # pprint(vars(rocket))
    start_time = time.time()
    while time.time() - start_time <= 30:
        elapsed_time = time.time() - start_time
        new_velo = update_state(elapsed_time)
        new_data = {
            "Time": elapsed_time,
            "Velocity": new_velo,
        }
        rocket.set(new_data)
        time.sleep(0.1)  # simulating one-second interval


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