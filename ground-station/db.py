import dash
from dash import dcc, html, Input, Output, callback
import plotly.graph_objects as go
from dummy_factory import DummyRocket

external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]

app = dash.Dash(__name__, external_stylesheets=external_stylesheets)
rocket = DummyRocket()

app.layout = html.Div([ #change layout to be: https://community.plotly.com/t/dash-bootstrap-components-and-flexbox/40816/2
    html.Div(
        [
            html.H1(
                children='Tufts SEDS Rocketry',
                style={
                    'textAlign': 'center',
                    'color':'black'
                }
            )
        ]
    ),
    html.Div([ #Outer Div
        html.Div( #Making vertical div
            [html.Div( #first few inner div
                [
                    html.H4(
                        children='Velocity',
                        style={
                            'textAlign': 'left',
                            'color':'black',
                            'background': 'red'
                        }) #Link for making guage: https://plotly.com/python/indicator/
                ]
            ),
            html.Div(
                [
                    html.H4(
                        children='Acceleration',
                        style={
                            'textAlign': 'left',
                            'color':'black',
                            'background': 'blue'
                        })]
            ),
            html.Div(
                [
                    html.H4(
                        children='Total Acceleration',
                        style={
                            'textAlign': 'left',
                            'color':'black',
                            'background': 'green'
                        }
                    )
                ]
            ),
            ], 
        ),

        html.Div( #Making vertical div
            [html.Div( #middle section display
                [
                    html.H4(
                        children='Barometer Graph',
                        style={
                            'textAlign': 'left',
                            'color':'black',
                            'background': 'Blue'
                        })
                ]
            ),
            html.Div(
                [
                    html.H4(
                        children='Map',
                        style={
                            'textAlign': 'left',
                            'color':'black',
                            'background': 'red'
                        })]
            ),
            ], 
        ),

        html.Div( #Making vertical div
            html.Div( #middle section display
                    html.H4(
                        children='Rocket Status',
                        style={
                            'textAlign': 'left',
                            'color':'black',
                            'background': 'yellow'
                        })
            ), 
        ),
    ]),
])


if __name__ == '__main__':
    app.run(debug=True)