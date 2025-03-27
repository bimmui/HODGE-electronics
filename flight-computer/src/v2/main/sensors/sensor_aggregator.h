#include "sensor_interface.h"

#define MAX_SENSORS 8

class ApoAggregator
{
public:
    ApoAggregator();

    bool addSensor(ApoSensor *sensor);
    void initializeSensors();
    sensor_data_snapshot generateSnapshot() const;

private:
    ApoSensor *sensors_[MAX_SENSORS];
    uint8_t num_sensors_;
    ts_sensor_data polling_data_;
};