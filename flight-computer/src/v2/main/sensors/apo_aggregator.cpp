#include "apo_aggregator.h"

ApoAggregator::ApoAggregator() : num_sensors_(0) {}

uint8_t ApoAggregator::getNumSensors()
{
    return num_sensors_;
}

bool ApoAggregator::addSensor(ApoSensor *sensor)
{
    if (num_sensors_ >= MAX_SENSORS)
        return false;
    sensors_[num_sensors_++] = sensor;
    return true;
}

char *ApoAggregator::initializeSensors()
{
    // TODO: add a check to make sure some sensors have been added to the aggregator
    //      using addSensor
    char *sensor_stats[num_sensors_];
    for (int i = 0; i < num_sensors_; i++)
    {
        sensor_status stat = sensors_[i]->initialize();
        char sensor_name[32];

        // formats the char string to be:
        // Sensor_[order in sensors_]_[sensor_type]_[device id]: status [sensor_status]
        snprintf(sensor_name, sizeof(sensor_name),
                 "Sensor_%d_%d_%u: status %d\n", i, sensors_[i]->getType(), sensors_[i]->getDevID(), static_cast<int>(stat));

        sensor_stats[i] = sensor_name;

        // TODO: do some profiling to figure out the stack size of each
        //      to make this a call to xTaskCreateStatic instead of xTaskCreate
        xTaskCreate(
            sensors_[i]->vreadTask,
            sensor_name,
            2048,
            static_cast<void *>(polling_data_),
            5, // priority
            NULL);
    }

    // what actually pulls events out of that queue and
    // invokes any handlers that are registered for them
    // this is mainly just for gps
    esp_event_loop_run();
    return stat;
}

sensor_data_snapshot ApoAggregator::generateSnapshot() const
{
    return sensor_data_snapshot{
        polling_data_.baro_altitude.load(),
        polling_data_.baro_temp.load(),
        polling_data_.baro_pressure.load(),

        polling_data_.gps_lat.load(),
        polling_data_.gps_lon.load(),
        polling_data_.gps_alt.load(),
        polling_data_.gps_speed.load(),
        polling_data_.gps_cog.load(),
        polling_data_.gps_mag_vari.load(),
        polling_data_.gps_num_sats.load(),
        polling_data_.gps_fix_status.load(),
        polling_data_.gps_year.load(),
        polling_data_.gps_month.load(),
        polling_data_.gps_day.load(),
        polling_data_.gps_hour.load(),
        polling_data_.gps_minute.load(),
        polling_data_.gps_second.load(),
        polling_data_.gps_fix_valid.load(),

        polling_data_.imu_accel_x.load(),
        polling_data_.imu_accel_y.load(),
        polling_data_.imu_accel_z.load(),
        polling_data_.imu_gyro_x.load(),
        polling_data_.imu_gyro_y.load(),
        polling_data_.imu_gyro_z.load(),
        polling_data_.imu_mag_x.load(),
        polling_data_.imu_mag_y.load(),
        polling_data_.imu_mag_z.load(),

        polling_data_.hg_accel_x.load(),
        polling_data_.hg_accel_y.load(),
        polling_data_.hg_accel_z.load(),

        polling_data_.temp_temp_c.load(),
        MILLIS()};
}

complete_sensor_data_snapshot ApoAggregator::generateCompleteSnapshot() const
{
    return complete_sensor_data_snapshot{
        polling_data_.baro_altitude.load(),
        polling_data_.baro_temp.load(),
        polling_data_.baro_pressure.load(),

        polling_data_.gps_lat.load(),
        polling_data_.gps_lon.load(),
        polling_data_.gps_alt.load(),
        polling_data_.gps_speed.load(),
        polling_data_.gps_cog.load(),
        polling_data_.gps_mag_vari.load(),
        polling_data_.gps_num_sats.load(),
        polling_data_.gps_fix_status.load(),
        polling_data_.gps_year.load(),
        polling_data_.gps_month.load(),
        polling_data_.gps_day.load(),
        polling_data_.gps_hour.load(),
        polling_data_.gps_minute.load(),
        polling_data_.gps_second.load(),
        polling_data_.gps_fix_valid.load(),

        polling_data_.imu_accel_x.load(),
        polling_data_.imu_accel_y.load(),
        polling_data_.imu_accel_z.load(),
        polling_data_.imu_gyro_x.load(),
        polling_data_.imu_gyro_y.load(),
        polling_data_.imu_gyro_z.load(),
        polling_data_.imu_mag_x.load(),
        polling_data_.imu_mag_y.load(),
        polling_data_.imu_mag_z.load(),

        polling_data_.hg_accel_x.load(),
        polling_data_.hg_accel_y.load(),
        polling_data_.hg_accel_z.load(),

        polling_data_.temp_temp_c.load(),
        MILLIS()};
}