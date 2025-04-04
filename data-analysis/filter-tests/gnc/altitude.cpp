/*
    altitude.cpp: Altitude estimation via barometer/accelerometer fusion
*/

#include "filters.h"
#include "algebra.h"
#include "altitude.h"

Estimator::Estimator(float sigma_accel, float sigma_gyro, float sigma_baro,
                     float accel_threshold)
    : kalman_(sigma_gyro), complementary_(sigma_accel, sigma_baro, accel_threshold)
{
}

void Estimator::estimate(float accel[3], float gyro[3], float mag[3], float baro_alt, uint32_t timestamp)
{
        float dt = (float)(timestamp - previous_time_) / 1000.0f;

        kalman_.predict(gyro, dt);
        kalman_.updateAccel(accel);
        kalman_.updateMag(mag);

        float vertical_accel = kalman_.calcVerticalAccel(accel);
        euler_angles attitude = kalman_.calcAttitude();
        comp_filter_results cfr = complementary_.estimate(baro_alt,
                                                          prev_altitude_,
                                                          prev_vertical_velocity_,
                                                          prev_vertical_accel_,
                                                          dt);

        // update values for next iteration
        copy_vector(prev_gyro_, gyro);
        copy_vector(prev_accel_, accel);
        prev_altitude_ = cfr.altitude;
        prev_vertical_velocity_ = cfr.vertical_velocity;
        prev_vertical_accel_ = vertical_accel;
        previous_time_ = timestamp;

        // updating results to return
        estimates_.angles = attitude;
        estimates_.cf_results = cfr;
        estimates_.vertical_accel = vertical_accel;
}

filter_estimates Estimator::getEstimates()
{
        return estimates_;
}

void Estimator::setInitTime(uint32_t time)
{
        previous_time_ = time;
}