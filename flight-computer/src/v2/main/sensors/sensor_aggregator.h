
#include "sensor_interface.h"

#define MAX_SENSORS 8

class ApoAggregator
{
public:
    ApoAggregator() : num_sensors_(0) {}

    bool add_sensor(ApoSensor *sensor)
    {
        if (num_sensors_ >= MAX_SENSORS)
            return false;
        sensors_[num_sensors_++] = sensor;
        return true;
    }

    void poll_all()
    {
        for (uint8_t i = 0; i < num_sensors_; ++i)
        {
            sensor_reading reading = sensors_[i]->read();
            handle_reading(reading);
        }
    }

private:
    // idk how tf im gonna do this with multiple threads
    void handle_reading(const sensor_reading &reading)
    {
        switch (reading.value.type)
        {
        case TEMPERATURE:
            process_temp(reading.value.data.temperature);
            break;
        case GPS_LOCATION:
            process_gps(reading.value.data.gps);
            break;
        }
    }

    ApoSensor *sensors_[MAX_SENSORS];
    uint8_t num_sensors_;
};