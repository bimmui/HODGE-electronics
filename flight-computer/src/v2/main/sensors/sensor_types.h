#include <atomic>

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

struct atomic_baro_data
{
    std::atomic<double> baro_altitude{0.0};
    std::atomic<float> baro_temp{0.0f};
    std::atomic<float> baro_pressure{0.0f};
}

struct atomic_gps_data
{
    std::atomic<double> gps_lat{0.0};
    std::atomic<double> gps_lon{0.0};
    std::atomic<double> gps_alt{0.0};
    std::atomic<float> gps_speed{0.0f};
    std::atomic<float> gps_cog{0.0f};
    std::atomic<float> gps_mag_vari{0.0f};
    std::atomic<int> gps_num_sats{0};
    std::atomic<int> gps_fix_status{0};
    std::atomic<int> gps_year{0};
    std::atomic<int> gps_month{0};
    std::atomic<int> gps_day{0};
    std::atomic<int> gps_hour{0};
    std::atomic<int> gps_minute{0};
    std::atomic<int> gps_second{0};
    std::atomic<bool> gps_fix_valid{false};
};

struct atomic_imu_data
{
    std::atomic<float> imu_accel_x{0.0f};
    std::atomic<float> imu_accel_y{0.0f};
    std::atomic<float> imu_accel_z{0.0f};
    std::atomic<float> imu_gyro_x{0.0f};
    std::atomic<float> imu_gyro_y{0.0f};
    std::atomic<float> imu_gyro_z{0.0f};
    std::atomic<float> imu_mag_x{0.0f};
    std::atomic<float> imu_mag_y{0.0f};
    std::atomic<float> imu_mag_z{0.0f};
    std::atomic<float> imu_temp{0.0f};
};

struct atomic_highg_data
{
    std::atomic<float> hg_accel_x{0.0f};
    std::atomic<float> hg_accel_y{0.0f};
    std::atomic<float> hg_accel_z{0.0f};
};

struct atomic_temp_data
{
    std::atomic<float> temp_temp_c{0.0f};
};

struct sensor_data_snapshot
{
    double baro_altitude;
    float baro_temp;
    float baro_pressure;

    double gps_lat;
    double gps_lon;
    double gps_alt;
    float gps_speed;
    float gps_cog;
    float gps_mag_vari;
    int gps_num_sats;
    int gps_fix_status;
    int gps_year;
    int gps_month;
    int gps_day;
    int gps_hour;
    int gps_minute;
    int gps_second;
    bool gps_fix_valid;

    float imu_accel_x;
    float imu_accel_y;
    float imu_accel_z;
    float imu_gyro_x;
    float imu_gyro_y;
    float imu_gyro_z;
    float imu_mag_x;
    float imu_mag_y;
    float imu_mag_z;

    float hg_accel_x;
    float hg_accel_y;
    float hg_accel_z;

    float temp_temp_c;
    unsigned long timestamp;
};

struct complete_sensor_data_snapshot
{
    sensor_data_snapshot base;

    float filter_yaw;
    float filter_pitch;
    float filter_roll;
    float filter_vertical_velocity;
    float filter_altitude;
    float filter_vertical_accel;

    complete_sensor_data_snapshot(sensor_data_snapshot const &s, float filter_yaw, float filter_pitch, float filter_roll,
                                  float filter_vertical_velocity, float filter_altitude, float filter_vertical_accel)
        : base(s), // copy‐construct the whole sensor_data_snapshot
          filter_yaw(filter_yaw), filter_pitch(filter_pitch), filter_roll(filter_roll),
          filter_vertical_velocity(filter_vertical_velocity), filter_altitude(filter_altitude),
          filter_vertical_accel(filter_vertical_accel)
    {
    }
};

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
            float temp;
        } imu;
        struct
        {
            float accel[3];
        } accelerometerHG;
        struct
        {
            float temp_c;
        } temp;
    } data;
};