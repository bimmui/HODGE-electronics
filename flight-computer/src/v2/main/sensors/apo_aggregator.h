#include "sensor_interface.h"
#include "icm20948.h"
#include "adxl375.h"
#include "bmp581.h"
#include "gps.h"
#include "tmp1075.h"

#define MAX_SENSORS 8

class ApoAggregator
{
public:
    ApoAggregator();

    bool addSensor(ApoSensor *sensor);
    void initializeSensors();
    uint8_t getNumSensors();
    sensor_data_snapshot generateSnapshot() const;

private:
    ApoSensor *sensors_[MAX_SENSORS];
    uint8_t num_sensors_;
    atomic_sensor_data polling_data_;
};