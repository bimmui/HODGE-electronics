import multiprocessing
import sys
import time
import datetime
import pdb
import random
#import serial
#import numpy as np
from queue import Empty
from pprint import pprint
from multiprocessing.managers import BaseManager
#import util


import dash
from dash import Dash, callback, html, dcc, callback_context, no_update
#import dash_bootstrap_components as dbc
#import dash_daq as daq
#import dash_leaflet as dl
import plotly.graph_objects as go
from dash.dependencies import Input, Output, State
#import plotly.express as px


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
        self.time_series_data = {"Time": [], "Internal Temp": [], "External Temp": []}

    # Need to make getter and setter funcs bc proxy objs are spawned by the manager,
    #   so direct access is not possible
    def set(self, min_dict):
        for key, value in min_dict.items():
            self.time_series_data[key].append(value)
        # print(self.time_series_data)

    def get(self):
        return self.time_series_data
    
    
def dashboard(rocket):
    external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

    app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

    app.layout = html.Div(
        html.Div(
            [
                html.H4("Rocket Launch Simulation"),
                dcc.Graph(id="live-update-temp-graph"),
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
        Output("live-update-temp-graph", "figure"),
        Input("interval-component", "n_intervals"),
    )
    def update_temp_graph_live(n):
        data = rocketo_proxy.get()

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

    app.run_server(debug=True)



def add_values(rocket):
    def update_state(elapsed_time):
        if elapsed_time <= 15:
            internal_temp = 13 + elapsed_time*5
            external_temp = 13 - elapsed_time*5
        else:
            time_since_burnout = elapsed_time - 5
            internal_temp = 13 + 5*15 - .5 * time_since_burnout
            external_temp = 13 - 5*15 + 3 * time_since_burnout
            

            
        return internal_temp, external_temp

    # pprint(vars(rocket))
    start_time = time.time()
    while time.time() - start_time <= 30:
        elapsed_time = time.time() - start_time
        new_int_temp, new_ext_temp = update_state(elapsed_time)
        new_data = {
            "Time": elapsed_time,
            "Internal Temp": new_int_temp,
            "External Temp": new_ext_temp,
            
        }
        rocket.set(new_data)
        time.sleep(0.1)  # simulating one-second interval

# Run the app
if __name__ == '__main__':
    BaseManager.register("DummyRocket", DummyRocket)
    manager = BaseManager()
    manager.start()
    inst = manager.DummyRocket()


    # p1 is adding values from dummyrocket
    p1 = multiprocessing.Process(target=add_values, args=[inst])

    # p2 is the dashboard that runs
    p2 = multiprocessing.Process(target=dashboard, args=[inst])

    # Database
    #p3 = multiprocessing.Process(target=populate_infludb, args=[inst])

    p1.start()
    time.sleep(3)
    p2.start()
    # p3.start()

    p1.join()
    p2.join()
    # p3.join()




