from dash import Dash, html, dcc, callback_context, no_update
import dash_bootstrap_components as dbc
import dash_daq as daq
from dash.dependencies import Input, Output, State

app = Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP])

# Initial states
INITIAL_STATE = {"ground_state": True, "camera_state": True, "units": True}


def create_bool_switches(id, left_label, right_label, switch_state, button_label=""):
    switch_row = dbc.Row(
        [
            dbc.Col(
                html.Label(
                    left_label,
                    style={"color": "white", "marginRight": "10px"},
                ),
                width="auto",
            ),
            dbc.Col(
                daq.BooleanSwitch(
                    id=id,
                    on=switch_state,
                    label=button_label,
                    color="#00cc96",
                    persistence=True,
                    persisted_props=["on"],
                    labelPosition="bottom",
                ),
                width="auto",
            ),
            dbc.Col(
                html.Label(
                    right_label,
                    style={"color": "white", "marginLeft": "10px"},
                ),
                width="auto",
            ),
        ],
        className="mb-3",
    )
    return switch_row


app.layout = html.Div(
    style={"backgroundColor": "#333"},  # Background for the entire app
    children=[
        dcc.Store(id="state-store", storage_type="local", data=INITIAL_STATE),
        dbc.Container(
            style={
                "backgroundColor": "transparent",
                "padding": "20px",
                "border": "2px solid red",
                "width": "300px",
            },
            children=[
                dbc.Stack(
                    [
                        html.H4(
                            "Current State",
                            id="current-state-text",
                            style={"color": "white", "textAlign": "center"},
                        ),
                        dbc.Form(
                            style={
                                "display": "flex",
                                "flexDirection": "column",
                                "alignItems": "center",
                                "justifyContent": "center",
                                "width": "100%",
                            },
                            children=[
                                create_bool_switches(
                                    "ground-state-switch",
                                    "POW ON",
                                    "LNCH RDY",
                                    False,
                                ),
                                create_bool_switches(
                                    "camera-switch", "Off", "On", False
                                ),
                                create_bool_switches(
                                    "units-switch", "Metric", "Imperial", True
                                ),
                            ],
                        ),
                    ],
                )
            ],
        ),
    ],
)


@app.callback(
    [
        Output("ground-state-switch", "on"),
        Output("camera-switch", "on"),
        Output("units-switch", "on"),
    ],
    [Input("state-store", "data")],
)
def load_initial_state(state_data):
    return state_data["ground_state"], state_data["camera_state"], state_data["units"]


@app.callback(
    Output("state-store", "data"),
    [
        Input("ground-state-switch", "on"),
        Input("camera-switch", "on"),
        Input("units-switch", "on"),
    ],
    State("state-store", "data"),
)
def update_state_store(ground_state, camera_state, units_state, state_data):
    ctx = callback_context

    if not ctx.triggered:
        return no_update

    triggered_id = ctx.triggered[0]["prop_id"].split(".")[0]

    if triggered_id == "ground-state-switch":
        state_data["ground_state"] = ground_state
    elif triggered_id == "camera-switch":
        state_data["camera_state"] = camera_state
    elif triggered_id == "units-switch":
        state_data["units"] = units_state

    return state_data


if __name__ == "__main__":
    app.run_server(debug=True)
