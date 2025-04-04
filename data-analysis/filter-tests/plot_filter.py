#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt


def main():
    # Read the filtered output produced by your program.
    filtered_df = pd.read_csv("output.csv")
    # Convert time from ms to seconds.
    filtered_df["Time (s)"] = filtered_df["Time (ms)"] / 1000.0

    # Read the original CSV data.
    original_df = pd.read_csv("../data/tf2.csv")
    # Assuming the original CSV uses 'time (ms)' and 'altitude (m)' for these fields.
    original_df["Time (s)"] = original_df["time (ms)"] / 1000.0

    # Graph 1: Estimated Altitude vs. Original Altitude
    plt.figure(figsize=(10, 6))
    plt.plot(
        filtered_df["Time (s)"],
        filtered_df["Altitude"],
        label="Estimated Altitude",
        color="blue",
        linewidth=2,
    )
    plt.plot(
        original_df["Time (s)"],
        original_df["altitude (m)"],
        label="Original Altitude",
        color="red",
        linestyle="--",
        linewidth=2,
    )
    plt.xlabel("Time (s)")
    plt.ylabel("Altitude (m)")
    plt.title("Estimated vs. Original Altitude")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    # plt.savefig("graph1_altitude.png")
    plt.show()

    # Graph 2: Estimated Vertical Velocity, Vertical Acceleration, and Original Altitude
    plt.figure(figsize=(10, 6))
    plt.plot(
        filtered_df["Time (s)"],
        filtered_df["Vertical Velocity"],
        label="Estimated Vertical Velocity",
        color="green",
        linewidth=2,
    )
    plt.plot(
        filtered_df["Time (s)"],
        filtered_df["Vertical Acceleration"],
        label="Estimated Vertical Acceleration",
        color="purple",
        linewidth=2,
    )
    plt.plot(
        original_df["Time (s)"],
        original_df["altitude (m)"],
        label="Original Altitude",
        color="red",
        linestyle="--",
        linewidth=2,
    )
    plt.xlabel("Time (s)")
    plt.ylabel("Measurements")
    plt.title("Vertical Velocity, Acceleration, and Original Altitude")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    # plt.savefig("graph2_velocity_acceleration_altitude.png")
    plt.show()

    # Graph 3: Subplots for Yaw, Pitch, and Roll
    fig, axes = plt.subplots(3, 1, figsize=(10, 10), sharex=True)
    axes[0].plot(
        filtered_df["Time (s)"], filtered_df["Yaw"], color="orange", linewidth=2
    )
    axes[0].set_ylabel("Yaw")
    axes[0].grid(True)
    axes[0].set_title("Yaw, Pitch, and Roll over Time")

    axes[1].plot(
        filtered_df["Time (s)"], filtered_df["Pitch"], color="brown", linewidth=2
    )
    axes[1].set_ylabel("Pitch")
    axes[1].grid(True)

    axes[2].plot(
        filtered_df["Time (s)"], filtered_df["Roll"], color="cyan", linewidth=2
    )
    axes[2].set_ylabel("Roll")
    axes[2].set_xlabel("Time (s)")
    axes[2].grid(True)

    plt.tight_layout()
    # plt.savefig("graph3_ypr.png")
    plt.show()


if __name__ == "__main__":
    main()
