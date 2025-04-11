#ifndef ICM20948_H
#define ICM20948_H

#include "peripherals/i2c_ex.h"
#include "sensor_interface.h"

typedef enum
{
    ACCEL_FS_2G = 0,  // Accelerometer full scale range is +/- 2g
    ACCEL_FS_4G = 1,  // Accelerometer full scale range is +/- 4g
    ACCEL_FS_8G = 2,  // Accelerometer full scale range is +/- 8g
    ACCEL_FS_16G = 3, // Accelerometer full scale range is +/- 16g
} accel_fs;

typedef enum
{
    GYRO_FS_250DPS = 0,  // Gyroscope full scale range is +/- 250 degree per sencond
    GYRO_FS_500DPS = 1,  // Gyroscope full scale range is +/- 500 degree per sencond
    GYRO_FS_1000DPS = 2, // Gyroscope full scale range is +/- 1000 degree per sencond
    GYRO_FS_2000DPS = 3, // Gyroscope full scale range is +/- 2000 degree per sencond
} gyro_fs;

typedef enum
{
    ICM20948_DLPF_0,
    ICM20948_DLPF_1,
    ICM20948_DLPF_2,
    ICM20948_DLPF_3,
    ICM20948_DLPF_4,
    ICM20948_DLPF_5,
    ICM20948_DLPF_6,
    ICM20948_DLPF_7,
    ICM20948_DLPF_OFF
} dlpf_mode;

struct ICM20948Config
{
    accel_fs accel_range;
    gyro_fs gyro_range;

    bool enable_accel_dlpf;
    bool enable_gyro_dlpf;
    dlpf_mode gyro_dlpf;
    dlpf_mode accel_dlpf;
};

class ICM20948
{
public:
    ICM20948(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
             uint16_t icm20948_address, uint32_t scl_clk_speed);
    ICM20948(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
             uint16_t icm20948_address, uint32_t scl_clk_speed,
             const ICM20948Config &cfg);
    ~ICM20948();

    sensor_status initialize() override;
    sensor_reading read() override;
    static void vreadTask(void *pvParameters) override;
    sensor_type getType() const override;
    uint8_t getDevID() override;
    void setCalibrationFactors(const float G_offset[3],
                               const float A_B[3], const float A_Ainv[3][3],
                               const float M_B[3], const float M_Ainv[3][3]);

private:
    ICM20948Config config_;

    float gyro_sensitivity_; // TODO: figure out if this is dps or rps
    float accel_sensitivity_;

    bool calibrated_;

    // bunch of matrixes and values to scale and calibrate imu data
    const float g_scale_;
    const float G_offset_[3];
    const float A_B_[3];
    const float A_Ainv_[3][3];
    const float M_B_[3];
    const float M_Ainv_[3][3];

    i2c_master_dev_handle_t icm20948_dev_handle_;
    i2c_master_dev_handle_t ak09916_dev_handle_;

    void configure() override;
    void wakeup();
    void sleep();
    void reset();

    void setBank(uint8_t bank);
    // void activateI2CBypass();

    void setGyroFS();
    gyro_fs getGyroFS();
    void setGyroSensitivity();

    void setAccelFS();
    accel_fs getAccelFS();
    void setAccelSensitivity();

    sensor_status enableAccelDLPF(bool enable);
    sensor_status enableGyroDLPF(bool enable);
};

#endif