/*
   filters.h: Filter class declarations
 */

#pragma once

// #include <cmath>
#include <math.h>
#include <stdint.h>

#include "algebra.h"

// find this empirically, might need to be updated onsite
// face comp towards true north/known orientation
// TODO: add function that calculates NED coords based on curr
//		location's declination, inclination, and magneteic field
#define Bx 1
#define By 0
#define Bz 0

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
	float H_a[3][4];

	// accelerometer measurement noise
	float R_a[3][3];

	// measurement jacobian for magnetometer
	float H_m[3][4];

	// magnetometer measurement noise
	float R_m[3][3];

	// looks something like this, do this in the init
	//  {
	//      {accel_noise, 0.f, 0.f},
	//      {0.f, accel_noise, 0.f},
	//      {0.f, 0.f, accel_noise}};

	// accelerometer measurement covariance
	// float R[3][3];
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
	void updateMag(const float mag[3]);

private:
	State curr_quat_;
	Ekf efk_vals_; // will hold curr vals
	float gyro_noise;
	const float B_E[3] = {Bx, By, Bz};

	// prediction steps that lead up to calling predict()
	State processFunction(const float gyro[3], float dt);
	void setQOrientation(float dt);
	void computeF(const float gyro[3], float dt, float F[4][4]);

	// update prediction with accelerometer data (nonlinear update step)
	void rotateGravity(float out[3]);
	void computeH_Accel();

	// update prediction with magnetometer data (nonlinear update step)
	void rotateMag(float out[3]);
	void computeH_Mag();
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

	float applyZUPT(float accel, float vel);
}; // Class ComplementaryFilter
