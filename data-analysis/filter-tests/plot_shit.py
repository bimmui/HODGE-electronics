#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt


def main():

    start_time = 1664000  # in ms for both
    end_time = 1750000
    df = pd.read_csv("../data/sim_data/highdata_sim_butter1.csv")
    df = df[(df["Time (ms)"] >= start_time) & (df["Time (ms)"] <= end_time)]

    time = (df["Time (ms)"] / 1000.0) - 1664

    accel_x = df["Accel X (m/s)"]
    accel_y = df["Accel Y (m/s)"]
    accel_z = df["Accel Z (m/s)"]

    gyro_x = df["Gyro X (rad/s)"]
    gyro_y = df["Gyro Y (rad/s)"]
    gyro_z = df["Gyro Z (rad/s)"]

    mag_x = df["Mag X (Gauss)"]
    mag_y = df["Mag Y (Gauss)"]
    mag_z = df["Mag Z (Gauss)"]

    altitude = df["Altitude"]
    vel_vert = df["Vertical Velocity"]
    accel_vert = df["Vertical Acceleration"]

    yaw = df["Yaw"]
    pitch = df["Pitch"]
    roll = df["Roll"]

    # 1) Yaw and Pitch vs Time (on the same plot)
    plt.figure()
    plt.plot(time, yaw, label="Yaw")
    # plt.plot(time, roll, label="Roll")
    plt.plot(time, pitch, label="Pitch")
    plt.xlabel("Time (s)")
    plt.ylabel("Angle (rad)")
    plt.title("Yaw and Pitch vs Time")
    plt.legend()
    plt.show()

    # 2) Altitude vs Time
    plt.figure()
    plt.plot(time, altitude)
    plt.xlabel("Time (ms)")
    plt.ylabel("Altitude")
    plt.title("Altitude vs Time")
    plt.show()

    # 3) Vertical Velocity and Vertical Acceleration vs Time
    plt.figure()
    plt.plot(time, vel_vert, label="Vertical Velocity")
    plt.plot(time, accel_vert, label="Vertical Acceleration")
    plt.xlabel("Time (ms)")
    plt.ylabel("Velocity / Acceleration")
    plt.title("Vertical Velocity & Vertical Acceleration vs Time")
    plt.legend()
    plt.show()

    # 4) Accel X, Y, Z vs Time in subplots
    fig, axes = plt.subplots(nrows=3, ncols=1, sharex=True)
    fig.suptitle("Accelerometer Data vs Time")

    axes[0].plot(time, accel_x)
    axes[0].set_ylabel("Accel X (m/s^2)")

    axes[1].plot(time, accel_y)
    axes[1].set_ylabel("Accel Y (m/s^2)")

    axes[2].plot(time, accel_z)
    axes[2].set_ylabel("Accel Z (m/s^2)")
    axes[2].set_xlabel("Time (s)")

    plt.show()

    # 5) Gyro X, Y, Z vs Time in subplots
    fig, axes = plt.subplots(nrows=3, ncols=1, sharex=True)
    fig.suptitle("Gyroscope Data vs Time")

    axes[0].plot(time, gyro_x)
    axes[0].set_ylabel("Gyro X (rad/s)")

    axes[1].plot(time, gyro_y)
    axes[1].set_ylabel("Gyro Y (rad/s)")

    axes[2].plot(time, gyro_z)
    axes[2].set_ylabel("Gyro Z (rad/s)")
    axes[2].set_xlabel("Time (s)")

    plt.show()

    # 6) Mag X, Y, Z vs Time in subplots
    fig, axes = plt.subplots(nrows=3, ncols=1, sharex=True)
    fig.suptitle("Magnetometer Data vs Time")

    axes[0].plot(time, mag_x)
    axes[0].set_ylabel("Mag X (Gauss)")

    axes[1].plot(time, mag_y)
    axes[1].set_ylabel("Mag Y (Gauss)")

    axes[2].plot(time, mag_z)
    axes[2].set_ylabel("Mag Z (Gauss)")
    axes[2].set_xlabel("Time (s)")

    plt.show()


if __name__ == "__main__":
    main()
