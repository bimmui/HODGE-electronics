/*
   filters.h: Filter class declarations
 */

#pragma once

// #include <cmath>
#include <math.h>
#include <stdint.h>

#include "algebra.h"

// simple 4D quaternion state
// maybe in the future do a 7D matrix
struct State
{
  float qw; // w
  float qx; // x
  float qy; // y
  float qz; // z
};

struct Ekf
{
  // 4x4 predicted covariance matrix
  float P[4][4];

  // Process noise 4x4
  float Q[4][4];

  // static gyro bias
  float gyro_bias[3];

  // measurement jacobian for accelerometer
  float H[3][4];

  // accelerometer measurement noise
  float R[3][3];

  // looks something like this, do this in the init
  //  {
  //      {accel_noise, 0.f, 0.f},
  //      {0.f, accel_noise, 0.f},
  //      {0.f, 0.f, accel_noise}};

  // accelerometer measurement covariance
  float R[3][3];
};

struct euler_angles
{
  float yaw;
  float pitch;
  float roll;
}; // these are actually Tait-Bryant angles :p

class ExtendedKalmanFilter
{
public:
  ExtendedKalmanFilter(float ca, float sigmaGyro, float sigmaAccel);

  float getVerticalAccel(const float accel[3]);
  euler_angles getAttitude();

private:
  State curr_quat_;
  Ekf efk_vals_; // will hold curr vals

  // prediction steps
  void setQOrientation(float gyro_noise, float dt);
  State processFunction(const float gyro[3], float dt);
  void computeF(const float gyro[3], float dt, float F[4][4]);
  void predict(const float gyro[3], float dt);

  // update prediction with accelerometer data (nonlinear update step)
  void rotateGravity(float out[3]);
  void computeH_Accel();
  void updateAccel(const float accel[3]);
};

class ComplementaryFilter
{

private:
  // filter gain
  float gain[2];
  // Zero-velocity update
  float accelThreshold;
  static const uint8_t ZUPT_SIZE = 12;
  uint8_t ZUPTIdx;
  float ZUPT[ZUPT_SIZE];

  float ApplyZUPT(float accel, float vel);

public:
  ComplementaryFilter(float sigmaAccel, float sigmaBaro, float accelThreshold);

  void estimate(float *velocity, float *altitude, float baroAltitude,
                float pastAltitude, float pastVelocity, float accel, float deltat);
}; // Class ComplementaryFilter
