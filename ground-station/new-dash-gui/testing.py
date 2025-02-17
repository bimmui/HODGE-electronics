from dash import Dash, html, Input, Output, callback
import dash_daq as daq

app = Dash()

app.layout = html.Div([
    daq.ToggleSwitch(
        id='our-toggle-switch',
        value=False
    ),
    html.Div(id='toggle-switch-result')
])


@callback(
    Output('toggle-switch-result', 'children'),
    Input('our-toggle-switch', 'value')
)
def update_output(value):
    return f'The switch is {value}.'

if __name__ == '__main__':
    app.run(debug=True)
