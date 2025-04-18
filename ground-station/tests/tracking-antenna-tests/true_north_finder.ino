
// New UW Mahony AHRS for the LSM9DS1  S.J. Remington 3/2021
// Requires the Sparkfun LSM9DS1 library
// Standard sensor orientation X North (yaw=0), Y West, Z up
// NOTE: Sensor X axis is remapped to the opposite direction of the "X arrow" on the Adafruit sensor breakout!

// New Mahony filter error scheme uses Up (accel Z axis) and West (= Acc X Mag) as the orientation reference vectors
// heavily modified from http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
// Both the accelerometer and magnetometer MUST be properly calibrated for this program to work.
// Follow the procedure described in http://sailboatinstruments.blogspot.com/2011/08/improved-magnetometer-calibration.html
// or in more detail, the tutorial https://thecavepearlproject.org/2015/05/22/calibrating-any-compass-or-accelerometer-for-arduino/
//
// To collect data for calibration, use the companion program LSM9DS1_cal_data
//
/*
  Adafruit 3V or 5V board
  Hardware setup: This library supports communicating with the
  LSM9DS1 over either I2C or SPI. This example demonstrates how
  to use I2C. The pin-out is as follows:
  LSM9DS1 --------- Arduino
   SCL ---------- SCL (A5 on older 'Duinos')
   SDA ---------- SDA (A4 on older 'Duinos')
   VIN ------------- 5V
   GND ------------- GND

   CSG, CSXM, SDOG, and SDOXM should all be pulled high.
   pullups on the ADAFRUIT breakout board do this.
*/
// The SFE_LSM9DS1 library requires both Wire and SPI be
// included BEFORE including the 9DS1 library.
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM9DS1.h>

//////////////////////////
// LSM9DS1 Library Init //
//////////////////////////
// default settings gyro  245 d/s, accel = 2g, mag = 4G
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();

// VERY IMPORTANT!
// magnetometers really do need to be calibrated inside their final operating environment
// These are the previously determined offsets and scale factors for accelerometer and magnetometer, using MPU9250_cal and Magneto
float Gscale = (M_PI / 180.0) * 0.00875; // 245 dps scale sensitivity = 8.75 mdps/LSB
float G_offset[3] = {0.0528, 0.0221, -0.0357};

// Accel scale 16457.0 to normalize
//  float A_B[3]
//  { -133.33,   72.29, -291.92};

// float A_Ainv[3][3]
// { {  1.00260,  0.00404,  0.00023},
//   {  0.00404,  1.00708,  0.00263},
//   {  0.00023,  0.00263,  0.99905}
// };

// Mag scale 3746.0 to normalize
float M_B[3]{32.67, 29.40, -7.12};

float M_Ainv[3][3]{{1.046, 0.045, -0.007},
                   {0.045, 0.971, 0.014},
                   {0.007, 0.014, 0.987}};

// local magnetic declination in degrees converted from degree minute seconds
float declination = -14.0631;

// These are the free parameters in the Mahony filter and fusion scheme,
// Kp for proportional feedback, Ki for integral
// Kp is not yet optimized. Ki is not used.
#define Kp 50.0
#define Ki 0.0

unsigned long now = 0, last = 0; // micros() timers for AHRS loop
float deltat = 0;                // loop time in seconds

#define PRINT_SPEED 300      // ms between angle prints
unsigned long lastPrint = 0; // Keep track of print time

// Vector to hold quaternion
static float q[4] = {1.0, 0.0, 0.0, 0.0};
static float yaw, pitch, roll; // Euler angle output

float vector_dot(float a[3], float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void vector_normalize(float a[3])
{
    float mag = sqrt(vector_dot(a, a));
    a[0] /= mag;
    a[1] /= mag;
    a[2] /= mag;
}

void get_scaled_IMU(float Gxyz[3], float Axyz[3], float Mxyz[3])
{

    lsm.read();
    sensors_event_t a, m, g, garbo;
    lsm.getEvent(&a, &m, &g, &garbo);
    byte i;
    float temp[3];
    Gxyz[0] = Gscale * (g.gyro.x - G_offset[0]);
    Gxyz[1] = Gscale * (g.gyro.y - G_offset[1]);
    Gxyz[2] = Gscale * (g.gyro.z - G_offset[2]);

    Axyz[0] = a.acceleration.x;
    Axyz[1] = a.acceleration.y;
    Axyz[2] = a.acceleration.z;
    Mxyz[0] = m.magnetic.x;
    Mxyz[1] = m.magnetic.y;
    Mxyz[2] = m.magnetic.z;

    // apply accel offsets (bias) and scale factors from Magneto

    // for (i = 0; i < 3; i++) temp[i] = (Axyz[i] - A_B[i]);
    // Axyz[0] = A_Ainv[0][0] * temp[0] + A_Ainv[0][1] * temp[1] + A_Ainv[0][2] * temp[2];
    // Axyz[1] = A_Ainv[1][0] * temp[0] + A_Ainv[1][1] * temp[1] + A_Ainv[1][2] * temp[2];
    // Axyz[2] = A_Ainv[2][0] * temp[0] + A_Ainv[2][1] * temp[1] + A_Ainv[2][2] * temp[2];
    vector_normalize(Axyz);

    // apply mag offsets (bias) and scale factors from Magneto

    for (int i = 0; i < 3; i++)
        temp[i] = (Mxyz[i] - M_B[i]);
    Mxyz[0] = M_Ainv[0][0] * temp[0] + M_Ainv[0][1] * temp[1] + M_Ainv[0][2] * temp[2];
    Mxyz[1] = M_Ainv[1][0] * temp[0] + M_Ainv[1][1] * temp[1] + M_Ainv[1][2] * temp[2];
    Mxyz[2] = M_Ainv[2][0] * temp[0] + M_Ainv[2][1] * temp[1] + M_Ainv[2][2] * temp[2];
    vector_normalize(Mxyz);
}

// Mahony orientation filter, assumed World Frame NWU (xNorth, yWest, zUp)
// Modified from Madgwick version to remove Z component of magnetometer:
// The two reference vectors are now Up (Z, Acc) and West (Acc cross Mag)
// sjr 3/2021
// input vectors ax, ay, az and mx, my, mz MUST be normalized!
// gx, gy, gz must be in units of radians/second
//
void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float deltat)
{
    // Vector to hold integral error for Mahony method
    static float eInt[3] = {0.0, 0.0, 0.0};
    // short name local variable for readability
    float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];
    float norm;
    float hx, hy, hz;             // observed West horizon vector W = AxM
    float ux, uy, uz, wx, wy, wz; // calculated A (Up) and W in body frame
    float ex, ey, ez;
    float pa, pb, pc;

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

    // Measured horizon vector = a x m (in body frame)
    hx = ay * mz - az * my;
    hy = az * mx - ax * mz;
    hz = ax * my - ay * mx;
    // Normalise horizon vector
    norm = sqrt(hx * hx + hy * hy + hz * hz);
    if (norm == 0.0f)
        return; // Handle div by zero

    norm = 1.0f / norm;
    hx *= norm;
    hy *= norm;
    hz *= norm;

    // Estimated direction of Up reference vector
    ux = 2.0f * (q2q4 - q1q3);
    uy = 2.0f * (q1q2 + q3q4);
    uz = q1q1 - q2q2 - q3q3 + q4q4;

    // estimated direction of horizon (West) reference vector
    wx = 2.0f * (q2q3 + q1q4);
    wy = q1q1 - q2q2 + q3q3 - q4q4;
    wz = 2.0f * (q3q4 - q1q2);

    // Error is the summed cross products of estimated and measured directions of the reference vectors
    // It is assumed small, so sin(theta) ~ theta IS the angle required to correct the orientation error.

    ex = (ay * uz - az * uy) + (hy * wz - hz * wy);
    ey = (az * ux - ax * uz) + (hz * wx - hx * wz);
    ez = (ax * uy - ay * ux) + (hx * wy - hy * wx);

    if (Ki > 0.0f)
    {
        eInt[0] += ex; // accumulate integral error
        eInt[1] += ey;
        eInt[2] += ez;
        // Apply I feedback
        gx += Ki * eInt[0];
        gy += Ki * eInt[1];
        gz += Ki * eInt[2];
    }

    // Apply P feedback
    gx = gx + Kp * ex;
    gy = gy + Kp * ey;
    gz = gz + Kp * ez;

    // update quaternion with integrated contribution
    //  small correction 1/11/2022, see https://github.com/kriswiner/MPU9250/issues/447
    gx = gx * (0.5 * deltat); // pre-multiply common factors
    gy = gy * (0.5 * deltat);
    gz = gz * (0.5 * deltat);
    float qa = q1;
    float qb = q2;
    float qc = q3;
    q1 += (-qb * gx - qc * gy - q4 * gz);
    q2 += (qa * gx + qc * gz - q4 * gy);
    q3 += (qa * gy - qb * gz + q4 * gx);
    q4 += (qa * gz + qb * gy - qc * gx);

    // Normalise quaternion
    norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
    norm = 1.0f / norm;
    q[0] = q1 * norm;
    q[1] = q2 * norm;
    q[2] = q3 * norm;
    q[3] = q4 * norm;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ; // wait for connection
    Serial.println();
    Serial.println("LSM9DS1 AHRS starting");

    if (!lsm.begin())
    {
        Serial.println("Failed to communicate with LSM9DS1.");
        while (1)
            ;
    }
    lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
    lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
    lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
}

void loop()
{
    static char updated = 0;                // flags for sensor updates
    static int loop_counter = 0;            // sample & update loop counter
    static float Gxyz[3], Axyz[3], Mxyz[3]; // centered and scaled gyro/accel/mag data

    loop_counter++;
    get_scaled_IMU(Gxyz, Axyz, Mxyz);

    // correct accel/gyro handedness
    // Note: the illustration in the LSM9DS1 data sheet implies that the magnetometer
    // X and Y axes are rotated with respect to the accel/gyro X and Y, but this is
    // not the case if Acc X axis is flipped to fix the handedness

    Axyz[0] = -Axyz[0]; // fix accel/gyro handedness
    Gxyz[0] = -Gxyz[0]; // must be done after offsets & scales applied to raw data

    now = micros();
    deltat = (now - last) * 1.0e-6; // seconds since last update
    last = now;

    MahonyQuaternionUpdate(Axyz[0], Axyz[1], Axyz[2], Gxyz[0], Gxyz[1], Gxyz[2],
                           Mxyz[0], Mxyz[1], Mxyz[2], deltat);

    if (millis() - lastPrint > PRINT_SPEED)
    {

        // Define Tait-Bryan angles, strictly valid only for approximately level movement
        // Standard sensor orientation : X magnetic North, Y West, Z Up (NWU)
        // this code corrects for magnetic declination.
        // Pitch is angle between sensor x-axis and Earth ground plane, toward the
        // Earth is positive, up toward the sky is negative. Roll is angle between
        // sensor y-axis and Earth ground plane, y-axis up is positive roll.
        // Tait-Bryan angles as well as Euler angles are
        // non-commutative; that is, the get the correct orientation the rotations
        // must be applied in the correct order.
        //
        // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // which has additional links.

        // WARNING: This angular conversion is for DEMONSTRATION PURPOSES ONLY. It WILL
        // MALFUNCTION for certain combinations of angles! See https://en.wikipedia.org/wiki/Gimbal_lock
        roll = atan2((q[0] * q[1] + q[2] * q[3]), 0.5 - (q[1] * q[1] + q[2] * q[2]));
        pitch = asin(2.0 * (q[0] * q[2] - q[1] * q[3]));
        yaw = atan2((q[1] * q[2] + q[0] * q[3]), 0.5 - (q[2] * q[2] + q[3] * q[3]));
        // to degrees
        yaw *= 180.0 / PI;
        pitch *= 180.0 / PI;
        roll *= 180.0 / PI;

        // http://www.ngdc.noaa.gov/geomag-web/#declination
        // conventional nav, yaw increases CW from North, corrected for local magnetic declination

        yaw = -(yaw + declination);
        if (yaw < 0)
            yaw += 360.0;
        if (yaw >= 360.0)
            yaw -= 360.0;

        Serial.print("ypr ");
        Serial.print(yaw, 0);
        Serial.print(", ");
        Serial.print(pitch, 0);
        Serial.print(", ");
        Serial.print(roll, 0);
        //      Serial.print(", ");  //prints 24 in 300 ms (80 Hz) with 16 MHz ATmega328
        //      Serial.print(loop_counter);  //sample & update loops per print interval
        loop_counter = 0;
        Serial.println();
        lastPrint = millis(); // Update lastPrint time
    }
}
// consider averaging a few headings for better results
