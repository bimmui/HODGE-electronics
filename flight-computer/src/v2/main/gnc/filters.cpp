/*
   filters.cpp: Filter class implementations
 */

// #include <cmath>
#include <stdlib.h> // XXX eventually use fabs() instead of abs() ?
#include <cstring>
#include "algebra.h"
#include "filters.h"

ExtendedKalmanFilter::ExtendedKalmanFilter(float gyro_noise)
{
	this->gyro_noise = gyro_noise;
}

void quaternion_to_rotation_matrix(State x, float R[3][3])
{
	float qw = x.qw;
	float qx = x.qx;
	float qy = x.qy;
	float qz = x.qz;

	float qw2 = qw * qw, qx2 = qx * qx, qy2 = qy * qy, qz2 = qz * qz;

	R[0][0] = qw2 + qx2 - qy2 - qz2;
	R[0][1] = 2.f * (qx * qy - qw * qz);
	R[0][2] = 2.f * (qx * qz + qw * qy);

	R[1][0] = 2.f * (qx * qy + qw * qz);
	R[1][1] = qw2 - qx2 + qy2 - qz2;
	R[1][2] = 2.f * (qy * qz - qw * qx);

	R[2][0] = 2.f * (qx * qz - qw * qy);
	R[2][1] = 2.f * (qy * qz + qw * qx);
	R[2][2] = qw2 - qx2 - qy2 + qz2;
}

State predict_quaternion(Ekf &ekf, State &x, const float gyro[3], float dt)
{
	// extract curr quaternion
	float qw = x.qw;
	float qx = x.qx;
	float qy = x.qy;
	float qz = x.qz;

	// // subtract bias from measured gyro
	// float gx = gyro[0] - ekf.gyro_bias[0];
	// float gy = gyro[1] - ekf.gyro_bias[1];
	// float gz = gyro[2] - ekf.gyro_bias[2];

	float gx = gyro[0];
	float gy = gyro[1];
	float gz = gyro[2];

	// quaternion derivative ~ 0.5 * Omega(gyro) * q
	float half_dt = 0.5f * dt;

	// Derivatives of q
	// dq0/dt = 0.5 * ( -qx*gx - qy*gy - qz*gz )
	float dq0 = -(qx * gx + qy * gy + qz * gz);
	// dq1/dt = 0.5 * (  qw*gx + qy*gz - qz*gy )
	float dq1 = (qw * gx + qy * gz - qz * gy);
	// dq2/dt = 0.5 * (  qw*gy - qx*gz + qz*gx )
	float dq2 = (qw * gy - qx * gz + qz * gx);
	// dq3/dt = 0.5 * (  qw*gz + qx*gy - qy*gx )
	float dq3 = (qw * gz + qx * gy - qy * gx);

	// euler forward integration (hmmm...)
	float qw_next = qw + half_dt * dq0;
	float qx_next = qx + half_dt * dq1;
	float qy_next = qy + half_dt * dq2;
	float qz_next = qz + half_dt * dq3;

	// norm quaternion
	float norm = sqrtf(qw_next * qw_next + qx_next * qx_next +
					   qy_next * qy_next + qz_next * qz_next);
	if (norm < 1e-12f)
	{
		// fallback to default if basicvally zero
		qw_next = 1;
		qx_next = 0;
		qy_next = 0;
		qz_next = 0;
		norm = 1;
	}
	qw_next /= norm;
	qx_next /= norm;
	qy_next /= norm;
	qz_next /= norm;

	// predicted state
	return State{qw_next, qx_next, qy_next, qz_next};
}

void build_omega(const float gyro[3], float Omega[4][4])
{
	float gx = gyro[0];
	float gy = gyro[1];
	float gz = gyro[2];

	// row 0
	Omega[0][0] = 0.f;
	Omega[0][1] = -gx;
	Omega[0][2] = -gy;
	Omega[0][3] = -gz;

	// row 1
	Omega[1][0] = gx;
	Omega[1][1] = 0.f;
	Omega[1][2] = gz;
	Omega[1][3] = -gy;

	// row 2
	Omega[2][0] = gy;
	Omega[2][1] = -gz;
	Omega[2][2] = 0.f;
	Omega[2][3] = gx;

	// row 3
	Omega[3][0] = gz;
	Omega[3][1] = gy;
	Omega[3][2] = -gx;
	Omega[3][3] = 0.f;
}

// called in each time step, essentially a random walk
// TODO: tune this mf, should gyro noise be static or dynamic?
void ExtendedKalmanFilter::setQOrientation(float dt)
{
	// Very rough approach: we guess Q ~ (dt^2 * gyro_noise^2) * I
	// not sure if time needs to be squred here tho
	// gyro_noise is in units: rad^2/s^2?
	float val = gyro_noise * gyro_noise * dt * dt;
	memset(efk_vals_.Q, 0, sizeof(float) * 16);
	efk_vals_.Q[0][0] = val;
	efk_vals_.Q[1][1] = val;
	efk_vals_.Q[2][2] = val;
	efk_vals_.Q[3][3] = val;
}

/// The actual function f(x,u):
/// xCurr -> xPred
State ExtendedKalmanFilter::processFunction(const float gyro[3], float dt)
{
	return predict_quaternion(efk_vals_, curr_quat_, gyro, dt);
}

/// Compute F = dF/dx. (4x4). Ignores normalization effect
void ExtendedKalmanFilter::computeF(const float gyro[3], float dt, float F[4][4])
{
	// F = I + (dt/2)*Omega(gyro)
	// not using curr_quat bc we have static gyro bias/dont store bias in 4x4

	// start with identity
	memset(F, 0, sizeof(float) * 16); // doin g this for safety
	F[0][0] = 1.f;
	F[1][1] = 1.f;
	F[2][2] = 1.f;
	F[3][3] = 1.f;

	// build Omega
	float Om[4][4];
	build_omega(gyro, Om);

	float half_dt = 0.5f * dt;

	// Add (dt/2)*Om to F
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			F[r][c] += half_dt * Om[r][c];
		}
	}
}

void ExtendedKalmanFilter::predict(const float gyro[3], float dt)
{
	// compute predicted quaternion
	State pred_quat = processFunction(gyro, dt);

	// setting up the process noise matrix (Q matrix)
	setQOrientation(dt);

	// compute F e.g. jacobian e.g. partial derivative of process function
	float F[4][4];
	computeF(gyro, dt, F);

	// fml
	// PPred = F * P * F^T + Q <-- note these are matrices
	float PPred[4][4];
	// rawdogging lin alg operations fuck that little library
	float tmp[4][4];
	memset(tmp, 0, sizeof(tmp)); // safety
	// tmp = F * ekf.P
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			for (int k = 0; k < 4; k++)
			{
				tmp[r][c] += F[r][k] * efk_vals_.P[k][c];
			}
		}
	}
	// PPred = tmp * F^T
	memset(PPred, 0, sizeof(PPred));
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			for (int k = 0; k < 4; k++)
			{
				PPred[r][c] += tmp[r][k] * F[c][k]; // note F^T => F[c][k]
			}
		}
	}
	// add Q
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			PPred[r][c] += efk_vals_.Q[r][c];
		}
	}

	// copy back to obj vals
	curr_quat_ = pred_quat; // update the state
	memcpy(efk_vals_.P, PPred, sizeof(PPred));
}

void ExtendedKalmanFilter::rotateGravity(float out[3])
{
	float R[3][3];
	quaternion_to_rotation_matrix(curr_quat_, R);
	// mult R by (0,0,9.81) or...
	// out = R * [0,0,9.81]
	// out[0] = (R[0][0] * 0) + (R[0][1] * 0) + (R[0][2] * 9.81);
	// out[1] = (R[1][0] * 0) + (R[1][1] * 0) + (R[1][2] * 9.81);
	// out[2] = (R[2][0] * 0) + (R[2][1] * 0) + (R[2][2] * 9.81);
	out[0] = R[0][2] * 9.81; // make 9.81 some constant
	out[1] = R[1][2] * 9.81;
	out[2] = R[2][2] * 9.81;
}

void ExtendedKalmanFilter::computeH_Accel()
{
	const float eps = 1e-5f;
	// baseline
	float h0[3];
	float qBase[4] = {curr_quat_.qw, curr_quat_.qx, curr_quat_.qy, curr_quat_.qz};
	rotateGravity(h0);

	for (int col = 0; col < 4; col++)
	{
		float backup = qBase[col];
		qBase[col] += eps;
		// TODO: renormalize small differences?
		float hPlus[3];
		rotateGravity(hPlus);

		// derivative
		for (int row = 0; row < 3; row++)
		{
			efk_vals_.H[row][col] = (hPlus[row] - h0[row]) / eps;
		}
		// revert
		qBase[col] = backup;
	}
}

void ExtendedKalmanFilter::updateAccel(const float accel[3])
{
	float h[3];
	rotateGravity(h); // h is the predicted accelerometer reading if there's no linear motion

	// H = dH/dx where H is "measurement jacobian for accelerometer"
	computeH_Accel();

	// 3D innovation/residual
	// y = z – h
	float y[3] = {accel[0] - h[0], accel[1] - h[1], accel[2] - h[2]};

	// Finding S - covariance of the innovation
	// S = H * PPred * H^T + R
	// tmp3x4 = H (3x4) * PPred (4x4) => 3x4
	// S(3x3) = tmp3x4(3x4) * H^T(4x3) => 3x3
	// then add R.
	float tmp3x4[3][4];
	memset(tmp3x4, 0, sizeof(tmp3x4)); // safety? should i even bother here

	// tmp3x4 = H * PPred
	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			for (int k = 0; k < 4; k++)
			{
				tmp3x4[r][c] += efk_vals_.H[r][k] * efk_vals_.P[k][c];
			}
		}
	}

	float S[3][3];
	memset(S, 0, sizeof(S)); // hmmmm

	// S = tmp3x4 * H^T
	// H^T => H[c][r]
	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			for (int k = 0; k < 4; k++)
			{
				S[r][c] += tmp3x4[r][k] * efk_vals_.H[c][k];
				// note: H^T means H[c][k] if H is 3x4
			}
		}
	}

	// Add R: S = S + R
	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			S[r][c] += efk_vals_.R[r][c];
		}
	}

	// Kalman gain
	// Compute K = PPred * H^T * inv(S)
	// tmp4x3 = PPred(4x4) * H^T(4x3) => 4x3
	// then K(4x3) = tmp4x3 * inv(S)(3x3)

	float tmp4x3[4][3];
	memset(tmp4x3, 0, sizeof(tmp4x3));

	// first multiply PPred * H^T => (4x4)*(4x3) => (4x3)
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			for (int k = 0; k < 4; k++)
			{
				tmp4x3[r][c] += efk_vals_.P[r][k] * efk_vals_.H[c][k];
			}
		}
	}

	// Invert S
	float Sinv[3][3];
	if (!invert3x3(Sinv, S))
	{
		// If invert fails, handle error (singular matrix).
		// For now let's just zero out K, skip update.
		memset(Sinv, 0, sizeof(Sinv));
		return;
	}

	// now multiply tmp4x3 by Sinv => K(4x3)
	float K[4][3];
	memset(K, 0, sizeof(K));

	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			for (int k = 0; k < 3; k++)
			{
				K[r][c] += tmp4x3[r][k] * Sinv[k][c];
			}
		}
	}

	// x (updated quaternion) = x (curr quaternion) + K*y
	float xUpd[4];
	float xCurr[4] = {curr_quat_.qw, curr_quat_.qx, curr_quat_.qy, curr_quat_.qz};
	for (int i = 0; i < 4; i++)
	{
		float sum = 0.f;
		for (int j = 0; j < 3; j++)
		{
			sum += K[i][j] * y[j];
		}
		xUpd[i] = xCurr[i] + sum;
	}
	// Renormalize quaternion
	float norm_q = sqrt(xUpd[0] * xUpd[0] + xUpd[1] * xUpd[1] + xUpd[2] * xUpd[2] + xUpd[3] * xUpd[3]);
	if (norm_q < 1e-12f)
	{
		// fallback to identity if degenerate
		xUpd[0] = 1.f;
		xUpd[1] = 0.f;
		xUpd[2] = 0.f;
		xUpd[3] = 0.f;
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			xUpd[i] /= norm_q;
		}
	}

	// storing updated quaternion in obj
	curr_quat_.qw = xUpd[0];
	curr_quat_.qx = xUpd[1];
	curr_quat_.qy = xUpd[2];
	curr_quat_.qz = xUpd[3];

	// PUpdated = (I - K*H)* PPred
	// we do tmp4x4 = K(4x3) * H(3x4) => (4x4)
	// then PUpdated = (I(4x4) - tmp4x4)*PPred

	float tmp4x4[4][4];
	memset(tmp4x4, 0, sizeof(tmp4x4));

	// tmp4x4 = K * H => (4x3)*(3x4) => (4x4)
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			for (int k = 0; k < 3; k++)
			{
				tmp4x4[r][c] += K[r][k] * efk_vals_.H[k][c];
			}
		}
	}

	// Build (I - tmp4x4)
	float IminusKH[4][4]; // dont roast on the name plz
	// TODO: am i wasting time looping? not sure if tmp4x4 is even that sparse
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			if (r == c)
				IminusKH[r][c] = 1.f - tmp4x4[r][c];
			else
				IminusKH[r][c] = 0.f - tmp4x4[r][c];
		}
	}

	// Finally, PUpdated = (IminusKH)*PPred
	float PUpdated[4][4];
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			for (int k = 0; k < 4; k++)
			{
				PUpdated[r][c] += IminusKH[r][k] * efk_vals_.P[k][c];
			}
		}
	}
	memcpy(efk_vals_.P, PUpdated, sizeof(PUpdated));
}

float ExtendedKalmanFilter::calcVerticalAccel(const float accel[3])
{
	float R[3][3];
	quaternion_to_rotation_matrix(curr_quat_, R);

	// rotate raw accel readings to Earth coords
	float aEarth[3];
	for (int i = 0; i < 3; i++)
	{
		aEarth[i] = R[i][0] * accel[0] + R[i][1] * accel[1] + R[i][2] * accel[2];
	}
	// subtract gravity
	aEarth[2] -= 9.81f; // Earth Z is "up" so reads +9.81 for downward gravity

	return aEarth[2];
}

euler_angles ExtendedKalmanFilter::calcAttitude()
{

	float qw = curr_quat_.qw;
	float qx = curr_quat_.qx;
	float qy = curr_quat_.qy;
	float qz = curr_quat_.qz;

	float roll = atan2((qw * qx + qy * qz),
					   0.5f - (qx * qx + qy * qy));
	float pitch = asin(2.0f * (qw * qy - qx * qz));
	float yaw = atan2((qx * qy + qw * qz),
					  0.5f - (qy * qy + qz * qz));

	return euler_angles{roll, pitch, yaw};
}

float ComplementaryFilter::ApplyZUPT(float accel, float vel)
{
	// first update ZUPT array with latest estimation
	ZUPT[ZUPTIdx] = accel;
	// and move index to next slot
	uint8_t nextIndex = (ZUPTIdx + 1) % ZUPT_SIZE;
	ZUPTIdx = nextIndex;
	// Apply Zero-velocity update
	for (uint8_t k = 0; k < ZUPT_SIZE; k++)
	{
		if (abs(ZUPT[k]) > accelThreshold)
			return vel;
	}
	return 0.0;
}

ComplementaryFilter::ComplementaryFilter(float sigma_accel, float sigma_baro, float accel_threshold)
{
	// Compute the filter gain
	gain_[0] = sqrt(2 * sigma_accel / sigma_baro);
	gain_[1] = sigma_accel / sigma_baro;
	// If acceleration is below the threshold the ZUPT counter
	// will be increased
	this->accel_threshold_ = accel_threshold_;
	// initialize zero-velocity update
	zupt_idx_ = 0;
	for (uint8_t k = 0; k < zupt_size_; k++)
	{
		zupt_[k] = 0;
	}
}

comp_filter_results ComplementaryFilter::estimate(float baro_altitude, float past_altitude,
												  float past_velocity, float accel, float dt)
{
	// Apply complementary filter
	*altitude = past_altitude + dt * (past_velocity + (gain[0] + gain[1] * dt / 2) * (baro_altitude - past_altitude)) + accel * pow(dt, 2) / 2;
	*velocity = past_velocity + dt * (gain[1] * (baro_altitude - past_altitude) + accel);
	// Compute zero-velocity update
	*velocity = ApplyZUPT(accel, *velocity);
}
