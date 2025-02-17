# Author(s):
#   Daniel Opara <hello@danielopara.me>


"""This module provides various classes for generating dummy data.

Used for testing the Dash app and how it would display data. Primarily used as a
prototype to model how data received via telemetry would be passed to the Dash app. 
"""

import time
import threading


class DummyRocket:
    """Creates objects that simulates the velocity and altitude of a rocket when launched.

    The rocket simulation is very rough but follows that of
    a generic parabolic curve given the time (x-value) the rocket should apogee.
    The ``duration`` parameter of ``launch()`` should be changed to account for any change in
    the burnout_time and apogee_time, but it isn't necessary.

    TODO: Add more attributes to the rocket that would be transmitted down to the ground station
    """

    def __init__(self):
        self.time_series_data = {"Time": [], "Altitude": [], "Velocity": []}
        self.gravity = 9.8
        self.burnout_time = 5
        self.apogee_time = 10
        self.launch_thread = threading.Thread(target=self.launch)
        self.launch_thread.daemon = True  # thread terminates when main program exits
        self.launch_thread.start()

    def launch(self, duration=15):
        start_time = time.time()

        while time.time() - start_time <= duration:
            elapsed_time = time.time() - start_time
            self.update_state(elapsed_time)
            new_data = {
                "Time": elapsed_time,
                "Altitude": self.altitude,
                "Velocity": self.velocity,
            }
            for key, value in new_data.items():
                self.time_series_data[key].append(value)
            time.sleep(0.1)  # simulating one-second interval

    def update_state(self, elapsed_time):
        if elapsed_time <= self.burnout_time:
            self.velocity = 50 * elapsed_time
            self.altitude = 0.5 * self.gravity * elapsed_time**2
        else:
            time_since_burnout = elapsed_time - self.burnout_time
            self.velocity = 50 * self.burnout_time - 9.8 * time_since_burnout
            self.altitude = (
                0.5 * self.gravity * self.burnout_time**2
                - 0.5 * 9.8 * time_since_burnout**2
            )

            if self.altitude < 0:
                self.altitude = 0
                self.velocity = 0