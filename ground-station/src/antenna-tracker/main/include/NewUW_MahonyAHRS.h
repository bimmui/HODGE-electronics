#include <cmath>
#include <cstdio>
#include "esp_log.h"
#include "LSM9DS1_ESP_IDF.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct
{
    float yaw;
    float pitch;
    float roll;
} euler_angles;

class MahonyAHRS
{
public:
    /**
     * @brief Class to initialize calibration parameters and filter constants to perform Mahony AHRS algo
     *
     * @param Gscale    Gyroscope scale factor (rad/s per LSB)
     * @param G_offset  Gyroscope offsets [gx_off, gy_off, gz_off]
     * @param A_B       Accelerometer bias [ax_off, ay_off, az_off]
     * @param A_Ainv    Inverse correction matrix for accelerometer
     * @param M_B       Magnetometer bias [mx_off, my_off, mz_off]
     * @param M_Ainv    Inverse correction matrix for magnetometer
     * @param declination   Local magnetic declination in degrees
     * @param Kp        Mahony proportional gain
     * @param Ki        Mahony integral gain (could prob keep this 0)
     */
    MahonyAHRS(float Gscale,
               const float G_offset[3],
               const float A_B[3],
               const float A_Ainv[3][3],
               const float M_B[3],
               const float M_Ainv[3][3],
               float declination,
               float Kp,
               float Ki)
        : Gscale_(Gscale),
          declination_(declination),
          Kp_(Kp),
          Ki_(Ki)
    {
        // store gyro offset
        for (int i = 0; i < 3; i++)
        {
            G_offset_[i] = G_offset[i];
            A_B_[i] = A_B[i];
            M_B_[i] = M_B[i];
        }

        // store scale matrices
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                A_Ainv_[r][c] = A_Ainv[r][c];
                M_Ainv_[r][c] = M_Ainv[r][c];
            }
        }

        // init quaternion to identity
        q_[0] = 1.0f;
        q_[1] = 0.0f;
        q_[2] = 0.0f;
        q_[3] = 0.0f;
    }

    /**
     * @brief Retrieve and process sensors_event_t data, then
     *        run Mahony filter to update internal quaternion.
     *
     * @param accel  Accelerometer event
     * @param mag    Magnetometer event
     * @param gyro   Gyroscope event
     * @param deltat Time step in seconds
     * @return euler_angles (yaw, pitch, roll in degrees)
     */
    euler_angles updateIMU(const sensors_event_t &accel,
                           const sensors_event_t &mag,
                           const sensors_event_t &gyro,
                           float deltat)
    {
        float Gxyz[3], Axyz[3], Mxyz[3];

        // scale and calibrate the raw data
        getScaledIMU(Gxyz, Axyz, Mxyz, accel, mag, gyro);

        // run the Mahony filter update
        euler_angles result;
        MahonyQuaternionUpdate(result,
                               Axyz[0], Axyz[1], Axyz[2],
                               Gxyz[0], Gxyz[1], Gxyz[2],
                               Mxyz[0], Mxyz[1], Mxyz[2],
                               deltat);

        return result;
    }

private:
    // Calibration parameters
    float Gscale_; // e.g. (M_PI/180.0)*0.00875
    float G_offset_[3];
    float A_B_[3];
    float A_Ainv_[3][3];
    float M_B_[3];
    float M_Ainv_[3][3];
    float declination_;

    // Mahony filter constants
    float Kp_;
    float Ki_;

    // Quaternion (q0, q1, q2, q3)
    float q_[4];

    // Integral error
    float eInt_[3];

    // Logging TAG
    static constexpr const char *TAG_ = "MahonyAHRS";

private:
    /**
     * @brief Helper func to normalize 3-element vector
     */
    void vector_normalize(float v[3])
    {
        // dot product
        float mag = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

        if (mag > 0.000001f) // Avoid division by zero
        {
            v[0] /= mag;
            v[1] /= mag;
            v[2] /= mag;
        }
    }

    /**
     * @brief Applies calibration to raw sensor data and normalizes readings
     */
    void getScaledIMU(float Gxyz[3], float Axyz[3], float Mxyz[3],
                      const sensors_event_t &a,
                      const sensors_event_t &m,
                      const sensors_event_t &g)
    {
        float temp[3];

        // Gyroscope (convert to rad/s)
        Gxyz[0] = Gscale_ * (g.gyro.x - G_offset_[0]);
        Gxyz[1] = Gscale_ * (g.gyro.y - G_offset_[1]);
        Gxyz[2] = Gscale_ * (g.gyro.z - G_offset_[2]);

        // Accelerometer (raw)
        Axyz[0] = a.acceleration.x;
        Axyz[1] = a.acceleration.y;
        Axyz[2] = a.acceleration.z;

        for (int i = 0; i < 3; i++)
        {
            temp[i] = Axyz[i] - A_B_[i];
        }
        Axyz[0] = A_Ainv_[0][0] * temp[0] + A_Ainv_[0][1] * temp[1] + A_Ainv_[0][2] * temp[2];
        Axyz[1] = A_Ainv_[1][0] * temp[0] + A_Ainv_[1][1] * temp[1] + A_Ainv_[1][2] * temp[2];
        Axyz[2] = A_Ainv_[2][0] * temp[0] + A_Ainv_[2][1] * temp[1] + A_Ainv_[2][2] * temp[2];

        vector_normalize(Axyz);

        // Magnetometer (raw)
        Mxyz[0] = m.magnetic.x;
        Mxyz[1] = m.magnetic.y;
        Mxyz[2] = m.magnetic.z;

        // Apply magnetometer calibration
        for (int i = 0; i < 3; i++)
            temp[i] = (Mxyz[i] - M_B_[i]);

        Mxyz[0] = M_Ainv_[0][0] * temp[0] + M_Ainv_[0][1] * temp[1] + M_Ainv_[0][2] * temp[2];
        Mxyz[1] = M_Ainv_[1][0] * temp[0] + M_Ainv_[1][1] * temp[1] + M_Ainv_[1][2] * temp[2];
        Mxyz[2] = M_Ainv_[2][0] * temp[0] + M_Ainv_[2][1] * temp[1] + M_Ainv_[2][2] * temp[2];

        vector_normalize(Mxyz);

        Axyz[0] = -Axyz[0]; // fix accel/gyro handedness
        Gxyz[0] = -Gxyz[0]; // must be done after offsets & scales applied to raw data
        // ESP_LOGI(TAG_, "Accel: %f, %f, %f | Gyro: %f, %f, %f | Mag: %f, %f, %f\n",
        //          a.acceleration.x, a.acceleration.y, a.acceleration.z, g.gyro.x, g.gyro.y,
        //          g.gyro.z, m.magnetic.x, m.magnetic.y, m.magnetic.z);
    }

    /**
     * @brief Mahony orientation filter, World Frame NWU (xNorth, yWest, zUp).
     *        Updates the internal quaternion and calculates Euler angles.
     */
    void MahonyQuaternionUpdate(euler_angles &result,
                                float ax, float ay, float az,
                                float gx, float gy, float gz,
                                float mx, float my, float mz,
                                float deltat)
    {
        // static float eInt_[3] = {0.0, 0.0, 0.0};
        // Local variable copies
        float q1 = q_[0],
              q2 = q_[1], q3 = q_[2], q4 = q_[3];

        // Auxiliary variables to avoid repeated arithmetic
        float q1q1 = q1 * q1;
        float q1q2 = q1 * q2;
        float q1q3 = q1 * q3;
        float q1q4 = q1 * q4;
        float q2q2 = q2 * q2;
        float q2q3 = q2 * q3;
        float q2q4 = q2 * q4;
        float q3q3 = q3 * q3;
        float q3q4 = q3 * q4;
        float q4q4 = q4 * q4;

        // Measured horizon vector = A x M  (in body frame)
        float hx = ay * mz - az * my;
        float hy = az * mx - ax * mz;
        float hz = ax * my - ay * mx;

        // Normalize horizon vector
        float norm = std::sqrt(hx * hx + hy * hy + hz * hz);
        if (norm < 1e-9f)
        {
            // avoiding division by zero
            ESP_LOGW(TAG_, "Horizon vector is zero length; skipping update.");
            return;
        }
        norm = 1.0f / norm;
        hx *= norm;
        hy *= norm;
        hz *= norm;

        // estimated direction of Up reference vector (in body frame)
        float ux = 2.0f * (q2q4 - q1q3);
        float uy = 2.0f * (q1q2 + q3q4);
        float uz = q1q1 - q2q2 - q3q3 + q4q4;

        // estimated direction of horizon (West) reference vector
        float wx = 2.0f * (q2q3 + q1q4);
        float wy = q1q1 - q2q2 + q3q3 - q4q4;
        float wz = 2.0f * (q3q4 - q1q2);

        // sum of cross products between measured & estimated Up and West = err
        // It is assumed small, so sin(theta) ~ theta IS the angle required to correct the orientation error.

        float ex = (ay * uz - az * uy) + (hy * wz - hz * wy);
        float ey = (az * ux - ax * uz) + (hz * wx - hx * wz);
        float ez = (ax * uy - ay * ux) + (hx * wy - hy * wx);

        // Apply integral feedback if Ki > 0
        if (Ki_ > 0.0f)
        {
            eInt_[0] += ex;
            eInt_[1] += ey;
            eInt_[2] += ez;

            gx += Ki_ * eInt_[0];
            gy += Ki_ * eInt_[1];
            gz += Ki_ * eInt_[2];
        }

        // Apply proportional feedback
        gx += Kp_ * ex;
        gy += Kp_ * ey;
        gz += Kp_ * ez;

        // Integrate rate of change of quaternion
        gx *= (0.5f * deltat);
        gy *= (0.5f * deltat);
        gz *= (0.5f * deltat);

        float qa = q1;
        float qb = q2;
        float qc = q3;
        q1 += (-qb * gx - qc * gy - q4 * gz);
        q2 += (qa * gx + qc * gz - q4 * gy);
        q3 += (qa * gy - qb * gz + q4 * gx);
        q4 += (qa * gz + qb * gy - qc * gx);

        // Normalize quaternion
        norm = std::sqrt(q1q1 + q2q2 + q3q3 + q4q4);
        if (norm > 1e-9f)
        {
            norm = 1.0f / norm;
            q_[0] = q1 * norm;
            q_[1] = q2 * norm;
            q_[2] = q3 * norm;
            q_[3] = q4 * norm;
        }
        else
        {
            // gotta avoid numerical blow-up
            ESP_LOGW(TAG_, "Quaternion norm too small, resetting to identity!");
            q_[0] = 1.0f;
            q_[1] = q_[2] = q_[3] = 0.0f;
        }

        // Convert updated quaternion to Euler angles (in degrees)
        float q0 = q_[0];
        float q1n = q_[1];
        float q2n = q_[2];
        float q3n = q_[3];

        float roll = std::atan2((q0 * q1n + q2n * q3n),
                                0.5f - (q1n * q1n + q2n * q2n));
        float pitch = std::asin(2.0f * (q0 * q2n - q1n * q3n));
        float yaw = std::atan2((q1n * q2n + q0 * q3n),
                               0.5f - (q2n * q2n + q3n * q3n));

        // Convert to degrees
        roll *= 180.0f / M_PI;
        pitch *= 180.0f / M_PI;
        yaw *= 180.0f / M_PI;

        // Adjust yaw to 0-360 range
        yaw = 180.0f + yaw;
        if (yaw < 0)
            yaw += 360.0f;
        if (yaw >= 360.0f)
            yaw -= 360.0f;

        // Apply (optional) local declination if you want heading relative to True North
        yaw += declination_;
        if (yaw >= 360.0f)
            yaw -= 360.0f;
        if (yaw < 0.0f)
            yaw += 360.0f;

        result.yaw = yaw;
        result.pitch = pitch;
        result.roll = roll;

        // ESP_LOGI(TAG_, "Yaw: %.2f, Pitch: %.2f, Roll: %.2f",
        //          result.yaw, result.pitch, result.roll);
    }
};