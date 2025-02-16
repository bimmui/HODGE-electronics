# Author(s):
#   Daniel Opara <hello@danielopara.me>

import dash
from dash import dcc, html, Input, Output, callback
import plotly.graph_objs as go
from dummy_factory import DummyRocket

external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

app = dash.Dash(__name__, external_stylesheets=external_stylesheets)
rocket = DummyRocket()

app.layout = html.Div(
    html.Div(
        [
            html.H4("Rocket Launch Simulation"),
            dcc.Graph(id="live-update-graph"),
            dcc.Interval(
                id="interval-component",
                interval=1 * 1000,  # in milliseconds
                n_intervals=0,
            ),
        ]
    )
)


@app.callback(
    Output("live-update-graph", "figure"),
    Input("interval-component", "n_intervals"),
    Input("live-update-graph", "hoverData"),
)
def update_graph_live(n, hoverData):
    data = rocket.time_series_data

    fig = go.Figure()
    fig.add_trace(
        go.Scatter(
            x=data["Time"],
            y=data["Altitude"],
            mode="lines+markers",
            name="Altitude",
            hoverinfo="none",
        )
    )
    fig.add_trace(
        go.Scatter(
            x=data["Time"],
            y=data["Velocity"],
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

    if hoverData:
        x_value = hoverData["points"][0]["x"]
        closest_index = min(
            range(len(data["Time"])), key=lambda i: abs(data["Time"][i] - x_value)
        )
        altitude_value = data["Altitude"][closest_index]
        velocity_value = data["Velocity"][closest_index]

        altitude_hoverlabel = go.layout.Annotation(
            x=x_value,
            y=altitude_value,
            text=f"Time: {x_value:.2f}<br>Altitude: {altitude_value:.2f}",
            showarrow=True,
            arrowhead=4,
            arrowsize=2,
            arrowwidth=2,
            arrowcolor="#636363",
            ax=0,
            ay=-40,
            bordercolor="#c7c7c7",
            borderwidth=2,
            borderpad=4,
            bgcolor="rgba(255,255,255,0.8)",
            opacity=0.8,
        )

        velocity_hoverlabel = go.layout.Annotation(
            x=x_value,
            y=velocity_value,
            text=f"Time: {x_value:.2f}<br>Velocity: {velocity_value:.2f}",
            showarrow=True,
            arrowhead=4,
            arrowsize=2,
            arrowwidth=2,
            arrowcolor="#636363",
            ax=0,
            ay=-40,
            bordercolor="#c7c7c7",
            borderwidth=2,
            borderpad=4,
            bgcolor="rgba(255,255,255,0.8)",
            opacity=0.8,
        )

        fig.update_layout(annotations=[altitude_hoverlabel, velocity_hoverlabel])

    return fig


if __name__ == "__main__":
    app.run(debug=True, port=8055)
