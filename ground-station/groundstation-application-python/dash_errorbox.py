import dash
from dash import html, dcc
import dash_bootstrap_components as dbc
from dash.dependencies import Input, Output, State

# Sample data
data = {"status": "OK", "errors": []}

# Initialize the app
app = dash.Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP])

# Define the layout
app.layout = dbc.Container(
    [
        dbc.Row(
            [
                dbc.Col(
                    dbc.Card(
                        [
                            dbc.CardHeader("System Status"),
                            dbc.CardBody(
                                [
                                    html.Div(id="status-message"),
                                    html.Ul(
                                        id="error-messages", style={"color": "red"}
                                    ),
                                    dcc.Interval(
                                        id="interval-component",
                                        interval=2000,  # Update every 2 seconds
                                        n_intervals=0,
                                    ),
                                ]
                            ),
                        ]
                    ),
                    width=6,
                ),
            ],
            justify="center",
            style={"marginTop": "50px"},
        )
    ],
    fluid=True,
)


# Callback to update the card content
@app.callback(
    [Output("status-message", "children"), Output("error-messages", "children")],
    [Input("interval-component", "n_intervals")],
)
def update_card(n_intervals):
    # simulate receiving data and updating status, this is gonna need a lot of changes
    if n_intervals % 5 == 0:  # Simulate an error every 5 intervals
        data["status"] = "Error"
        data["errors"].append("System overheating!")
    else:
        data["status"] = "OK"
        data["errors"].clear()

    status_message = f"Status: {data['status']}"
    error_messages = [html.Li(error) for error in data["errors"]]

    return status_message, error_messages


if __name__ == "__main__":
    app.run_server(debug=True)
