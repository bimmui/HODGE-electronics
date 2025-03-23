typedef enum
{
    TEMPERATURE,
    BMP,
    GPS,
    IMU,
    ACCELEROMETER
} sensor_type;

typedef enum
{
    SENSOR_OK,
    SENSOR_ERR, // generic error
    SENSOR_ERR_BAD_HEALTH,
    SENSOR_ERR_INIT,
    SENSOR_ERR_READ,
    SENSOR_ERR_CALIB,
    SENSOR_ERR_TASK,
} sensor_status;

struct sensor_value
{
    sensor_type type;
    union
    {
        struct
        {
            double altitude;
            float temp, pressure;
        } bmp;
        struct
        {
            double lat, lon, alt;
            float speed, cog, mag_vari;
            int num_sats, fix_status;
            int year, month, day, hour, minute, second;
            bool fix_valid;
        } gps;
        struct
        {
            float accel[3], gyro[3], mag[3];
        } imu;
        struct
        {
            float accel[3];
        } accelerometerHG;
    } data;
};