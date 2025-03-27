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

struct comp_filter_results
{
	float vertical_velocity;
	float altitude;
};

class ExtendedKalmanFilter
{
public:
	ExtendedKalmanFilter(float gyro_noise);

	// call these for actual values
	float calcVerticalAccel(const float accel[3]);
	euler_angles calcAttitude();

	// call prediction first
	void predict(const float gyro[3], float dt);

	// then update the state with other sensor values
	void updateAccel(const float accel[3]);

private:
	State curr_quat_;
	Ekf efk_vals_; // will hold curr vals
	float gyro_noise;

	// prediction steps
	State processFunction(const float gyro[3], float dt);
	void setQOrientation(float dt);
	void computeF(const float gyro[3], float dt, float F[4][4]);

	// update prediction with accelerometer data (nonlinear update step)
	void rotateGravity(float out[3]);
	void computeH_Accel();
};

class ComplementaryFilter
{

public:
	ComplementaryFilter(float sigma_accel, float sigma_baro, float accel_threshold);

	comp_filter_results estimate(float baro_altitude, float past_altitude,
								 float past_velocity, float accel, float dt);

private:
	// filter gain
	float gain_[2];
	// Zero-velocity update
	float accel_threshold_;
	static const uint8_t zupt_size_ = 32;
	uint8_t zupt_idx_;
	float zupt_[zupt_size_];

	float ApplyZUPT(float accel, float vel);
}; // Class ComplementaryFilter
