enum sensor_type
{
    TEMPERATURE,
    BMP,
    GPS,
    IMU,
    ACCELEROMETER
};

typedef enum
{
    SENSOR_OK,
    SENSOR_ERR_INIT,
    SENSOR_ERR_READ,
    SENSOR_ERR_CALIB
} sensor_status;

struct sensor_value
{
    sensor_type type;
    union
    {
        struct
        {
            double altitude, pressure;
            float temp;
        } bmp;
        struct
        {
            double lat, lon, alt;
            float speed, cog, mag_vari;
            int num_sats, fix_status, fix_valid;
            int year, month, day, hour, minute, second;
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