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
import dash_daq as daq
from dash import dcc, html, Input, Output, callback, State
import plotly.graph_objs as go
from plotly.subplots import make_subplots

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
        self.time_series_data = {
            "Time": [],
            "Positions": [[37.7749, -122.4194],], # start in San Fran for pos
            "Internal Temp": [],
            "External Temp": [],
            "Acceleration": [],
            "Velocity": [],
            "Altitude": [],
            "State":"Current State"
        }  
        self.start_time = time.time()

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
    
    def gen_new_ava(self):
        """Dummy Rocket method for generating new acceleration, velocity, and altitude"""
        elapsed_time = time.time() - self.start_time
        # time_since_burnout = elapsed_time - 5

        # Use a simple parabolic trajectory, with a peak of 3050m at 50 seconds
        accel = -2.44 + elapsed_time #random.uniform(-1,1)
        velocity = -2.44 * elapsed_time + 122
        altitude = -1.22 * elapsed_time **2 + 122*elapsed_time

        if altitude <= 0:
            velocity = 0
            accel = 0
            altitude = 0
        # if elapsed_time <= 50:
        #     accel = -8
        #     velocity = -8 * elapsed_time + 400
        #     altitude = -4 * elapsed_time **2 + 400*elapsed_time
        # else:
        #     accel = -9.8
        #     velocity = 6.5 * 15 - 2 * time_since_burnout
        #     altitude = 0.5 * 9.8 * 5**2 - 0.5 * 9.8 * time_since_burnout**2
            
        #     if altitude < 0:
        #         altitude = 0
        #         # velocity = 0
        #         # accel = 0
        #     if velocity > 175:
        #         velocity = 100
                
        new_data = {
            "Time": elapsed_time,
            "Acceleration": accel,
            "Velocity" : velocity,
            "Altitude": altitude
            
        }
        self.set(new_data)
        
        return 0
         

def dashboard(rocket):
    dbc_css = "https://cdn.jsdelivr.net/gh/AnnMarieW/dash-bootstrap-templates/dbc.min.css"
    app = dash.Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP, dbc_css])

    # deep copy of rocket, pass by value
    rocketo_proxy = rocket
    data = rocket.get()

    # Initial positions
    positions = data["Positions"]
    #print(f"line 65, last pos: {positions[-1]}")
    rocketState = "State: " + data["State"]

    app.layout = html.Div([

        html.H4("Rocket Launch Simulation", style={"text-align":"center"}),

        # Here is a diagram of the layout:
        # +------------+------------+------------+
        # | Alt Gauge  | Vel Gauge  | Map        |
        # |            |            |            |    --> This is R1
        # |            |            |            |
        # |            |            |            |
        # +------------+------------+------------+
        # | State      |  Int-Temp  | Data       |
        # |------------|  Ex-Temp   |            |
        # |            |            +------------+    --> This is R2, whose boxes are split to fit more data
        # |Accel Gauge |            | Toggle     |
        # +------------+------------+------------+


        # First Row: Alt gauge, vel gauge, map
        dbc.Row([
            dbc.Col([
                dcc.Graph(id="live-update-vel-gauge",style={"height": "35vh", "margin":".5vh"},),
            ]),
            dbc.Col(dcc.Graph(id="live-update-alt-gauge",style={"height": "35vh", "margin":".5vh"})),
            dbc.Col(
            [
                
                dl.Map(
                    [
                        dl.TileLayer(),
                        dl.FullScreenControl(),
                        dl.Polyline(id="path", positions=positions),

                    ],
                    center=[37.7749, -122.4194],
                    zoom=5,
                    style={"height": "35vh", "margin":".5vh"},
                    id="map",
                ),
                
                dcc.Interval(
                    id="interval-component",
                    interval=1 * 1000,  # in milliseconds
                    n_intervals=0,
                ),
            ]
            ),
        ]),


        # Second Row: See diagram for depiction
        dbc.Row([
            dbc.Col([
                dbc.Card(dbc.CardBody(
                    html.H1(rocketState,style={"height": "10vh","text-align":"center"})
                )),
                dbc.Row(
                    dcc.Graph(id="live-update-accel-gauge", style={'height': '35vh'})
                ),
            ]),

            dbc.Col([
                dbc.Row(
                    dcc.Graph(id="live-update-temp-graph",style={'height': '55vh'})
                ),
                

            ]),

            dbc.Col([
                dbc.Card(
                    dbc.CardBody([
                        html.H4("GPS Data", className="card-title", style={"text-align":"center"}),
                        html.P([
                            html.Span(
                                "Coordinates: ",
                                style={"fontWeight": "bold", "textDecoration": "underline"},
                            ),
                            html.Span(id="coordinates", className="card-text"),
                        ]),
                        html.P([
                            html.Span(
                                "Altitude (m): ",
                                style={"fontWeight": "bold", "textDecoration": "underline"},
                            ),
                            html.Span(id="altitude-m", className="card-text"),
                        ]),
                        html.P([
                            html.Span(
                                "Altitude (ft): ",
                                style={"fontWeight": "bold", "textDecoration": "underline"},
                            ),
                            html.Span(id="altitude-ft", className="card-text"),
                        
                        ]),
                        html.P([
                            html.Span(
                                "RSSI: ",
                                style={"fontWeight": "bold", "textDecoration": "underline"},
                            ),
                            html.Span(id="signal_quality", className="card-text"),    
                        ]),
                        html.P([
                            html.Span(
                                "GPS Fix: ",
                                style={"fontWeight": "bold", "textDecoration": "underline"},
                            ),
                            html.Span(id="gps_fix", className="card-text"),
                            
                        ]),
                        html.P([
                            html.Span(
                                "Antenna Status: ",
                                style={"fontWeight": "bold", "textDecoration": "underline"},
                            ),
                            html.Span(id="antenna_status", className="card-text"),
                            
                        ]),
                    ]),
                    style={"height":"40vh","margin":"1vh"}
                ),

                dbc.Card(dbc.CardBody(
                    daq.BooleanSwitch(
                        id="switch-id",
                        on=False,
                        label="Label here",
                        color="#00cc96",
                        persistence=True,
                        persisted_props=["on"],
                        labelPosition="bottom",
                    )
                ))
                    
                
                
            ]),
        ])
    ])

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
        
        Output("live-update-temp-graph", "figure"),
        
        Input("interval-component", "n_intervals"),
    )
    def update_temp_graphs_live(n):
        rocketo_proxy.generate_new_temp()
        data = rocketo_proxy.get()

        tempFig = make_subplots(
            rows=2,
            cols=1,
            subplot_titles=("Int Temp", "Ext Temp")
        )

        tempFig.add_trace(
            go.Scatter(
                x=data["Time"],
                y=data["Internal Temp"],
                mode="lines+markers",
                name="Internal Temp (°C)",
                hoverinfo="none",
            ),
            row=1,col=1
        )
        tempFig.add_trace(
            go.Scatter(
                x=data["Time"],
                y=data["External Temp"],
                mode="lines+markers",
                name="External Temp (°C)",
                hoverinfo="none",
            ),
            row=2, col=1
        )

        tempFig.update_layout(
            title_text="Temperatures",
            title_x=0.5,
            #height=600,
            showlegend=False,
            hovermode="closest",
            # legend=dict(
            #     x=0.5,  # Center horizontally
            #     y=-0.2,  # Move below the plot
            #     xanchor="center",  # Anchor position to center
            #     yanchor="top"
            # )
        )

        tempFig.update_xaxes(title_text="Time (s)", row=2, col=1)
        tempFig.update_yaxes(title_text="Degrees (°C)", row=1, col=1)
        tempFig.update_yaxes(title_text="Degrees (°C)", row=2, col=1)


        return tempFig
    

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

    def update_gps_card_status(n):
    # Simulate data update, replace with actual data retrieval

        data = rocketo_proxy.get()
        current_positions = data["Positions"]
        altitude = data["Altitude"]
        #print(f"latest pos: {current_positions[-1]}")


        new_values = {
            "coordinates": f"Lat: {current_positions[-1][0]:.2f}, Lon: {current_positions[-1][1]:.2f}",
            "altitude-m": f'{data["Altitude"][-1]:.2f} m',
            "altitude-ft": f'{data["Altitude"][-1]*3.28:.2f} ft',
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
            Output("live-update-alt-gauge", "figure"),
        ],
        
        [
            Input("interval-component", "n_intervals"),
            Input("switch-id", "on")
        ]
    )

    def update_all_gauges_live(n, is_imperial):
        
        
        rocketo_proxy.gen_new_ava()
        data = rocketo_proxy.get()

        
        valList = [
                # Acceleration Markers, last one is latest acel data pt
                   -15,40,-9.8, (data["Acceleration"][-1]),
                # Velocity Markers, last one is latest vel data pt
                   -400,400,(data["Velocity"][-1]),
                # Altitude Markers, last one is latest alt data pt
                   4000,100,(data["Altitude"][-1])
                ]
        if is_imperial:
            for x in range(0,10):
                if x > 3:
                    valList[x] *=3.28
                else:
                    valList[x] = valList[x]/9.8
        
        data = rocketo_proxy.get()
        accelFig = go.Figure(
            go.Indicator(
                mode="gauge+number",
                value=round(valList[3],3),
                
                gauge={
                    "shape": "angular",
                    "axis": {"range": [valList[0], valList[1]]},
                    "bar": {"color": "rgba(0,0,0,0)"},  # Use black to hide the bar
                    "steps" : [
                        {'range': [valList[0], valList[2]], 'color': 'red'},
                        {'range': [valList[2], 0], 'color': 'yellow'},
                        {'range': [0, valList[1]], 'color': 'green'},
                    ],
                    "bgcolor": "#FFA07A",
                    "threshold": {
                        "line": {"color": "#800020", "width": 6},
                        "thickness": 0.75,
                        "value": valList[3],
                    },
                    
                },
                domain={"x": [0, 1], "y": [0, 1]},
                title=f'Total Acceleration ({"G" if is_imperial else "m/s^2"})'
            )
        )

        accelFig.update_layout(
            paper_bgcolor='lightblue',
            margin=dict(l=25, r=50, t=35, b=10),
        )
        

        velFig = go.Figure(
            go.Indicator(
                mode="gauge+number",
                value=round(valList[6],3),
                
                gauge={
                    "shape": "angular",
                    "axis": {"range": [valList[4], valList[5]]},
                    "bar": {"color": "rgba(0,0,0,0)"},  # Use black to hide the bar
                    "steps" : [
                        {'range': [valList[4], 0], 'color': 'lightgray'},
                        {'range': [0, valList[5]], 'color': 'green'}
                    ],
                    "bgcolor": "#FFA07A",
                    "threshold": {
                        "line": {"color": "#cef2ef", "width": 6},
                        "thickness": 0.75,
                        "value": valList[6],
                    },
                },
                
                domain={"x": [0, 1], "y": [0, 1]},
                title=f'Total Velocity ({"ft/s" if is_imperial else "m/s"})'
            )
        )

        velFig.update_layout(
            paper_bgcolor=('orange' if is_imperial else 'yellow'),
            margin=dict(l=25, r=50, t=35, b=10),
        )


        altFig = go.Figure(
            go.Indicator(
                mode="gauge+number",
                value=round(valList[9],3),
                
                gauge={
                    "shape": "angular",
                    "axis": {"range": [0, valList[7]]},
                    "bar": {"color": "rgba(0,0,0,0)"},  # Use black to hide the bar
                    "steps" : [
                        {'range': [0, valList[8]], 'color': 'green'}
                    ],
                    "bgcolor": "#FFA07A",
                    "threshold": {
                        "line": {"color": "#cef2ef", "width": 6},
                        "thickness": 0.75,
                        "value": valList[9],
                    },
                },
                
                domain={"x": [0, 1], "y": [0, 1]},
                title=f'Altitude ({"ft" if is_imperial else "m"})'
            )
        )

        altFig.update_layout(
            paper_bgcolor=('orange' if is_imperial else 'yellow'),
            margin=dict(l=25, r=50, t=35, b=10),
        )

        #accelFig.update_traces(value=valList[3])
        #velFig.update_traces(value=data["Velocity"][-1])
        #altFig.update_traces(value=data["Altitude"][-1])
        #return data["Acceleration"]
        # return positions

        return accelFig, velFig, altFig
    

    @app.callback(
        
        Output("switch-id","label"),
        
        Input("switch-id","on"),
    )

    def update_bool_label(on):
        return "Imperial" if on else "Metric"#, on


    app.run_server(debug=True, port=8060)

def add_values(rocket):
    

    # pprint(vars(rocket))
    # simulate receiving new coordinates
    data = rocket.get()
    current_positions = data["Positions"]
    
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

    