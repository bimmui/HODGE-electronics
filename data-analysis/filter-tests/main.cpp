#include <memory>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "altitude.h"
#include "StateDetermination.h"
#include "Butterworth.h"

struct SensorData
{
    uint32_t time;
    float ax, ay, az;
    float gx, gy, gz;
    float mx, my, mz;
    float altitude;
};

template <int MaxOrder>
struct FilterInstance
{
    // The Butterworth filter object for up to MaxOrder
    Dsp::Butterworth::LowPass<MaxOrder> filter;

    // The state for the filter, per stage.
    // Note: "typename" + "template" is required because
    // this is a dependent template name in C++.
    typename Dsp::CascadeStages<MaxOrder>::template State<Dsp::DirectFormII> state;
};
class MultiChannelFilter
{
public:
    MultiChannelFilter(size_t num_channels, double fs, double cutoff, int order)
        : fs(fs), cutoff(cutoff), order(order)
    {
        filters.resize(num_channels);
        for (auto &f : filters)
        {
            f.filter.setup(order, fs, cutoff);
            f.state.reset();
        }
    }

    void processRow(const std::vector<double> &input, std::vector<double> &output)
    {
        output.resize(input.size());
        for (size_t i = 0; i < input.size(); ++i)
        {
            if (i < filters.size())
            {
                double sample = input[i];
                filters[i].filter.process(1, &sample, filters[i].state);
                output[i] = sample;
            }
            else
            {
                output[i] = input[i];
            }
        }
    }

    void reset()
    {
        for (auto &f : filters)
        {
            f.filter.setup(order, fs, cutoff);
            f.state.reset();
        }
    }

private:
    std::vector<FilterInstance<10>> filters;
    double fs;
    double cutoff;
    int order;
};

int main()
{
    std::ifstream file("../data/carmf5_HIGHDATA.CSV");
    std::string line;
    std::vector<SensorData> sensor_data;
    StateDeterminer state_determiner = StateDeterminer();

    // sampling frequency (Hz)
    // last i remembered from some informal profiling on the pancakes
    const double fs = 50.0;
    const double cutoff_hz = 10.0; // cutoff frequency
    const int order = 4;           // filter order
    const int num_channels = 10;

    // we got ten stuff we wanna filter
    MultiChannelFilter filter(num_channels, fs, cutoff_hz, order);

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
            outfile << "Time (ms),Accel X (m/s),Accel Y (m/s),Accel Z (m/s),Gyro X (rad/s),Gyro Y (rad/s),Gyro Z (rad/s),Mag X (Gauss),Mag Y (Gauss),Mag Z (Gauss),Altitude,Vertical Velocity,Vertical Acceleration,Yaw,Pitch,Roll\n";

            // Use sensorData vector here
            std::cout << "Starting to write to file" << std::endl;
            // for (const auto &data : sensor_data)
            // {
            //     float accel_data[3] = {float(data.ax), float(data.ay), float(data.az)};
            //     float gyro_data[3] = {data.gx, data.gy, data.gz};
            //     float mag_data[3] = {data.mx, data.gy, data.mz};
            //     filter_estimates vals = state_determiner.determineState(accel_data, gyro_data, mag_data, data.altitude, data.time);
            //     outfile << data.time << "," << vals.cf_results.altitude << "," << vals.cf_results.vertical_velocity << "," << vals.vertical_accel << "," << vals.angles.yaw << "," << vals.angles.pitch << "," << vals.angles.roll << "\n";
            // }

            for (const auto &data : sensor_data)
            {
                std::vector<double> input_vals;
                std::vector<double> output_vals;

                input_vals.push_back(data.ax);
                input_vals.push_back(data.ay);
                input_vals.push_back(data.az);
                input_vals.push_back(data.gx);
                input_vals.push_back(data.gy);
                input_vals.push_back(data.gz);
                input_vals.push_back(data.mx);
                input_vals.push_back(data.my);
                input_vals.push_back(data.mz);
                input_vals.push_back(data.altitude);

                filter.processRow(input_vals, output_vals);

                float accel_data[3] = {
                    static_cast<float>(output_vals[0]),
                    static_cast<float>(output_vals[1]),
                    static_cast<float>(output_vals[2])};
                float gyro_data[3] = {
                    static_cast<float>(output_vals[3]),
                    static_cast<float>(output_vals[4]),
                    static_cast<float>(output_vals[5])};
                float mag_data[3] = {
                    static_cast<float>(output_vals[6]),
                    static_cast<float>(output_vals[7]),
                    static_cast<float>(output_vals[8])};

                float filtered_altitude = static_cast<float>(output_vals[9]);

                filter_estimates vals = state_determiner.determineState(accel_data, gyro_data, mag_data, filtered_altitude, data.time);

                outfile << data.time << ",";
                for (int i = 0; i < 9; i++)
                {
                    outfile << output_vals[i];
                    outfile << ",";
                }
                outfile << vals.cf_results.altitude << "," << vals.cf_results.vertical_velocity << "," << vals.vertical_accel << "," << vals.angles.yaw << "," << vals.angles.pitch << "," << vals.angles.roll << "\n";
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