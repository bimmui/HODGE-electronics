/*
    altitude.h: Altitude estimation via barometer/accelerometer fusion
*/

#pragma once

#include "filters.h"
#include "algebra.h"

struct filter_estimates
{
  euler_angles angles;
  comp_filter_results cf_results;
  float vertical_accel;
};

class Estimator
{
public:
  Estimator(float sigma_accel, float sigma_gyro, float sigma_baro,
            float accel_threshold);

  void estimate(float accel[3], float gyro[3], float mag[3], float baro_alt, uint32_t timestamp);

  filter_estimates getEstimates();

  void setInitTime(uint32_t time);

private:
  // For computing the sampling period
  uint32_t previous_time_;
  // required filters for altitude and vertical velocity estimation
  ExtendedKalmanFilter kalman_;
  ComplementaryFilter complementary_;
  filter_estimates estimates_;
  // required parameters for the filters used for the estimations
  // sensor's standard deviations
  // float sigma_accel_;
  // float sigma_gyro_;
  // float sigma_baro_;
  // Acceleration markov chain model state transition constant
  // float ca_;
  // Zero-velocity update acceleration threshold
  // float cf_accel_threshold_;
  // gravity
  // float g = 9.81;
  // Estimated past vertical acceleration
  float prev_vertical_accel_ = 0;
  float prev_vertical_velocity_ = 0;
  float prev_altitude_ = 0;
  float prev_gyro_[3] = {0, 0, 0};
  float prev_accel_[3] = {0, 0, 0};
}; // class AltitudeEstimator
