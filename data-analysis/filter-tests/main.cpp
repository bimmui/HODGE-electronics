#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "altitude.h"
#include "StateDetermination.h"

struct SensorData
{
    uint32_t time;
    float ax, ay, az;
    float gx, gy, gz;
    float mx, my, mz;
    float altitude;
};

int main()
{
    std::ifstream file("../data/tf2.csv");
    std::string line;
    std::vector<SensorData> sensor_data;
    StateDeterminer state_determiner = StateDeterminer();
    if (file.is_open())
    {
        std::string header;
        // Read the header line.
        std::getline(file, header);

        const int time_idx = 1;
        const int alt_idx = 7;
        const int accelx_idx = 12;
        const int accely_idx = 13;
        const int accelz_idx = 14;
        const int magx_idx = 15;
        const int magy_idx = 16;
        const int magz_idx = 17;
        const int gyrox_idx = 18;
        const int gyroy_idx = 19;
        const int gyroz_idx = 20;

        std::string line;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> fields;

            // Split the line by commas.
            while (std::getline(ss, token, ','))
            {
                fields.push_back(token);
            }

            if (fields.size() > gyroz_idx)
            {
                SensorData data;
                try
                {
                    data.time = static_cast<uint32_t>(std::stoul(fields[time_idx]));
                    data.altitude = std::stof(fields[alt_idx]);
                    data.ax = std::stof(fields[accelx_idx]);
                    data.ay = std::stof(fields[accely_idx]);
                    data.az = std::stof(fields[accelz_idx]);
                    data.mx = std::stof(fields[magx_idx]);
                    data.my = std::stof(fields[magy_idx]);
                    data.mz = std::stof(fields[magz_idx]);
                    data.gx = std::stof(fields[gyrox_idx]);
                    data.gy = std::stof(fields[gyroy_idx]);
                    data.gz = std::stof(fields[gyroz_idx]);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Conversion error: " << e.what() << std::endl;
                    continue;
                }
                sensor_data.push_back(data);
            }
        }
        file.close();

        std::ofstream outfile("output.csv");
        if (outfile.is_open())
        {
            // Write the header
            outfile << "Time (ms),Altitude,Vertical Velocity,Vertical Acceleration,Yaw,Pitch,Roll\n";

            // Use sensorData vector here
            std::cout << "Starting to write to file" << std::endl;
            for (const auto &data : sensor_data)
            {
                float accel_data[3] = {float(data.ax), float(data.ay), float(data.az)};
                float gyro_data[3] = {data.gx, data.gy, data.gz};
                float mag_data[3] = {data.mx, data.gy, data.mz};
                filter_estimates vals = state_determiner.determineState(accel_data, gyro_data, mag_data, data.altitude, data.time);
                outfile << data.time << "," << vals.cf_results.altitude << "," << vals.cf_results.vertical_velocity << "," << vals.vertical_accel << "," << vals.angles.yaw << "," << vals.angles.pitch << "," << vals.angles.roll << "\n";
            }

            outfile.close();
            std::cout << "Finished write to file" << std::endl;
        }
        else
        {
            std::cerr << "Unable to open output file!" << std::endl;
        }
    }
    else
    {
        std::cerr << "Unable to open file!" << std::endl;
    }

    return 0;
}