#include "sensor_interface.h"
#include "gnc/StateDetermination.h"

#define MAX_SENSORS 8

class ApoAggregator
{
public:
    ApoAggregator();

    bool addSensor(ApoSensor *sensor);
    void initializeSensors();
    uint8_t getNumSensors();
    complete_sensor_data_snapshot generateCompleteSnapshot() const;

private:
    ApoSensor *sensors_[MAX_SENSORS];
    uint8_t num_sensors_;
    atomic_baro_data baro_polling_data_;
    atomic_gps_data gps_polling_data_;
    atomic_imu_data imu_polling_data_;
    atomic_highg_data hg_polling_data_;
    atomic_temp_data temp_polling_data_;

    sensor_data_snapshot generateSnapshot() const;
};