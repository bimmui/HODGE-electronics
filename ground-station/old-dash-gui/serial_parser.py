# Author(s):
#   Daniel Opara <hello@danielopara.me>

import serial
import numpy as np

def get_sensor_data(serial_obj):
    """
    Get accel, gyro and barometer data from serial
    """
    raw_data = serial_obj.readline().rstrip().split(",")
    data = map(float, raw_data)
    # split into gyro and accel readings
    accel = np.array(data[:3]) * g
    # account for gyro bias
    gyro = np.array(data[3:6])
    # pressure
    baro = data[-2]
    return accel, gyro, baro


# Serial parameters
PORT = "/dev/ttyACM0"
BAUDRATE = 115200  # TODO: increase this

serial_com = serial.Serial(PORT, BAUDRATE)
