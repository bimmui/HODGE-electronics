#include "icm20948.h"

/* ICM20948 registers */
#define ICM20948_GYRO_CONFIG_1 (0x01)
#define ICM20948_ACCEL_CONFIG (0x14)
#define ICM20948_USER_CTRL (0x03)
#define ICM20948_ACCEL_XOUT_H (0x2D)
#define ICM20948_GYRO_XOUT_H (0x33)
#define ICM20948_TEMP_XOUT_H (0x39) // TODO: use this to get the temp
#define ICM20948_PWR_MGMT_1 (0x06)
#define ICM20948_WHO_AM_I (0x00)
#define ICM20948_WHO_AM_I_VAL (0xEA)
#define ICM20948_REG_BANK_SEL (0x7F)

/* ICM20948 masks */
#define REG_BANK_MASK (0x30) // only BIT5 and BIT4 is used for bank selection
#define FULLSCALE_SET_MASK (0x39)
#define FULLSCALE_GET_MASK (0x06)
#define DLPF_SET_MASK (0xC7)
#define DLPF_ENABLE_MASK (0x07)
#define DLPF_DISABLE_MASK (0xFE)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ICM20948::ICM20948(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                   uint16_t adxl375_address, uint32_t scl_clk_speed) : config_{ACCEL_FS_8G, GYRO_FS_1000DPS, false, false, ICM20948_DLPF_OFF, ICM20948_DLPF_OFF}
{
    this->icm20948_dev_handle_ = i2c_create_device(port, addr_len, adxl375_address, scl_clk_speed);
    this->ak09916_dev_handle_ = nullptr;
    this->calibrated_ = false;
}

ICM20948::ICM20948(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                   uint16_t adxl375_address, uint32_t scl_clk_speed,
                   const ICM20948Config &cfg) : config_(cfg)
{
    this->icm20948_dev_handle_ = i2c_create_device(port, addr_len, adxl375_address, scl_clk_speed);
    this->ak09916_dev_handle_ = nullptr;
}

ICM20948::~ICM20948()
{
    i2c_remove_device(icm20948_dev_handle_);
    if (ak09916_dev_handle_)
        i2c_remove_device(ak09916_dev_handle_);
}

uint8_t ICM20948::getDevID()
{
    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_WHO_AM_I, tmp, sizeof(tmp));
    return tmp[0];
}

sensor_type ICM20948::getType() const
{
    return IMU;
}

void ICM20948::setBank(uint8_t bank)
{
    assert(bank <= 3 && "Bank index out of range");
    bank = (bank << 4) & REG_BANK_MASK;

    const uint8_t reg_and_data[] = {ICM20948_REG_BANK_SEL, bank};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::wakeup()
{
    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_PWR_MGMT_1, tmp, sizeof(tmp));
    tmp[0] &= (~BIT6); // esp_bit_defs.h

    const uint8_t reg_and_data[] = {ICM20948_PWR_MGMT_1, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::sleep()
{
    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_PWR_MGMT_1, tmp, sizeof(tmp));
    tmp[0] |= BIT6;

    const uint8_t reg_and_data[] = {ICM20948_PWR_MGMT_1, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::reset()
{
    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_PWR_MGMT_1, tmp, sizeof(tmp));
    tmp[0] |= BIT7; // esp_bit_defs.h

    const uint8_t reg_and_data[] = {ICM20948_PWR_MGMT_1, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::setGyroFS()
{
    setBank(2);

    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_GYRO_CONFIG_1, tmp, sizeof(tmp));

    tmp[0] &= FULLSCALE_SET_MASK;
    tmp[0] |= (config_.gyro_range << 1);

    const uint8_t reg_and_data[] = {ICM20948_GYRO_CONFIG_1, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

gyro_fs ICM20948::getGyroFS()
{
    setBank(2);

    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_GYRO_CONFIG_1, tmp, sizeof(tmp));

    tmp[0] &= FULLSCALE_GET_MASK;
    tmp[0] >>= 1;
    gyro_fs gfs = static_cast<gyro_fs>(tmp[0]); // casting just to be safe, no implicit shit
    return gfs;
}

void ICM20948::setAccelFS()
{

    setBank(2);
    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_ACCEL_CONFIG, tmp, sizeof(tmp));

    tmp[0] &= FULLSCALE_SET_MASK;
    tmp[0] |= (config_.accel_range << 1);

    const uint8_t reg_and_data[] = {ICM20948_ACCEL_CONFIG, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

accel_fs ICM20948::getAccelFS()
{
    setBank(2);

    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_ACCEL_CONFIG, tmp, sizeof(tmp));

    tmp[0] &= FULLSCALE_GET_MASK;
    tmp[0] >>= 1;
    accel_fs acce_fs = static_cast<accel_fs>(tmp[0]);
    return acce_fs;
}

void ICM20948::setGyroSensitivity()
{
    gyro_fs gyro_fullscale = getGyroFS();

    switch (gyro_fs)
    {
    case GYRO_FS_250DPS:
        gyro_sensitivity_ = 131.f;
        break;
    case GYRO_FS_500DPS:
        gyro_sensitivity_ = 65.5f;
        break;
    case GYRO_FS_1000DPS:
        gyro_sensitivity_ = 32.8f;
        break;
    case GYRO_FS_2000DPS:
        gyro_sensitivity_ = 16.4f;
        break;
    default:
        break;
    }
}

void ICM20948::setAccelSensitivity()
{
    accel_fs accel_fullscale = getAccelFS();

    switch (accel_fullscale)
    {
    case ACCEL_FS_2G:
        accel_sensitivity_ = 16384.f;
        break;
    case ACCEL_FS_4G:
        accel_sensitivity_ = 8192.f;
        break;
    case ACCEL_FS_8G:
        accel_sensitivity_ = 4096.f;
        break;
    case ACCEL_FS_16G:
        accel_sensitivity_ = 2048.f;
        break;
    default:
        break;
    }
}

sensor_status ICM20948::enableAccelDLPF(bool enable)
{
    // checking if user gave some dumbass configs
    if ((enable) && (config_.accel_dlpf == ICM20948_DLPF_OFF))
        return SENSOR_ERR_INIT;
    if ((!enable) && (config_.accel_dlpf != ICM20948_DLPF_OFF))
        return SENSOR_ERR_INIT;

    setBank(2);

    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_ACCEL_CONFIG, tmp, sizeof(tmp));

    if (enable)
        tmp[0] |= DLPF_ENABLE_MASK;
    else
        tmp[0] &= DLPF_DISABLE_MASK;

    const uint8_t reg_and_data1[] = {ICM20948_ACCEL_CONFIG, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data1, sizeof(reg_and_data1));

    // setting up the low pass filter or not depending on user
    tmp[0] = 0;
    i2c_read(icm20948_dev_handle_, ICM20948_ACCEL_CONFIG, tmp, sizeof(tmp));

    tmp[0] &= DLPF_SET_MASK;
    tmp[0] |= config_.accel_dlpf << 3;

    const uint8_t reg_and_data[] = {ICM20948_ACCEL_CONFIG, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

sensor_status ICM20948::enableGyroDLPF(bool enable)
{
    if ((enable) && (config_.gyro_dlpf == ICM20948_DLPF_OFF))
        return SENSOR_ERR_INIT;
    if ((!enable) && (config_.gyro_dlpf != ICM20948_DLPF_OFF))
        return SENSOR_ERR_INIT;

    setBank(2);

    uint8_t tmp[1] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_GYRO_CONFIG_1, tmp, sizeof(tmp));

    if (enable)
        tmp[0] |= DLPF_ENABLE_MASK;
    else
        tmp[0] &= DLPF_DISABLE_MASK;

    const uint8_t reg_and_data2[] = {ICM20948_GYRO_CONFIG_1, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data2, sizeof(reg_and_data2));

    // setting up the actual low pass filter

    tmp[0] = 0;
    i2c_read(icm20948_dev_handle_, ICM20948_GYRO_CONFIG_1, tmp, sizeof(tmp));

    tmp[0] &= DLPF_SET_MASK;
    tmp[0] |= config_.gyro_dlpf << 3;

    const uint8_t reg_and_data[] = {ICM20948_GYRO_CONFIG_1, tmp[0]};
    i2c_write(icm20948_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::setCalibrationFactors(const float G_offset[3],
                                     const float A_B[3], const float A_Ainv[3][3],
                                     const float M_B[3], const float M_Ainv[3][3])
{
    // this is converting to redians, idk if we want that
    g_scale_ = (M_PI / 180.0f) * gyro_sensitivity_;
    G_offset_ = G_offset;
    A_B_ = A_B;
    A_Ainv_ = A_Ainv;
    M_B_ = M_B;
    M_Ainv_ = M_Ainv;

    calibrated_ = true;
}

void ICM20948::configure()
{
    reset();
    vTaskDelay(500 / portTICK_PERIOD_MS); // change this

    setGyroFS();
    setAccelFS();
    setGyroSensitivity();
    setAccelSensitivity();

    // gotta do something with the return values here
    enableAccelDLPF(config_.enable_accel_dlpf);
    enableGyroDLPF(config_.enable_gyro_dlpf);
}

sensor_status ICM20948::initialize()
{
    if (getDevID() != ICM20948_WHO_AM_I_VAL)
        return SENSOR_ERR_INIT;

    configure();

    return SENSOR_OK;
}

static void ICM20948::vreadTask(void *pvParameters)
{
    while (true)
    {
        sensor_reading curr_reading = read();
        ts_sensor_data *data = static_cast<ts_sensor_data *>(pvParameters);

        if (curr_reading.value.type != IMU)
            continue;

        if (curr_reading.status == SENSOR_OK)
        {
            data->imu_accel_x.store(curr_reading.value.data.imu.accel[0], std::memory_order_relaxed);
            data->imu_accel_y.store(curr_reading.value.data.imu.accel[1], std::memory_order_relaxed);
            data->imu_accel_z.store(curr_reading.value.data.imu.accel[2], std::memory_order_relaxed);
            data->imu_gyro_x.store(curr_reading.value.data.imu.gyro[0], std::memory_order_relaxed);
            data->imu_gyro_y.store(curr_reading.value.data.imu.gyro[1], std::memory_order_relaxed);
            data->imu_gyro_z.store(curr_reading.value.data.imu.gyro[2], std::memory_order_relaxed);
        }
    }
}

sensor_reading ICM20948::read()
{
    setBank(0);
    sensor_reading result;
    result.value.type = IMU;

    // getting acceleration data first
    uint8_t data_rd[6] = {0};
    esp_err_t success = i2c_read(icm20948_dev_handle_, ICM20948_ACCEL_XOUT_H, data_rd, sizeof(data_rd));

    if (success != ESP_OK)
    {
        result.status = SENSOR_ERR_READ;
        return result;
    }

    int16_t raw_accel_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    int16_t raw_accel_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    int16_t raw_accel_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));

    float accel_x = raw_accel_x / accel_sensitivity_;
    float accel_y = raw_accel_y / accel_sensitivity_;
    float accel_z = raw_accel_z / accel_sensitivity_;

    // now we get gyro shit
    uint8_t data_rd1[6] = {0};
    i2c_read(icm20948_dev_handle_, ICM20948_GYRO_XOUT_H, data_rd, sizeof(data_rd));

    int16_t raw_gyro_x = (int16_t)((data_rd1[0] << 8) + (data_rd1[1]));
    int16_t raw_gyro_y = (int16_t)((data_rd1[2] << 8) + (data_rd1[3]));
    int16_t raw_gyro_z = (int16_t)((data_rd1[4] << 8) + (data_rd1[5]));

    float gyro_x = raw_gyro_x / gyro_sensitivity_;
    float gyro_y = raw_gyro_y / gyro_sensitivity_;
    float gyro_z = raw_gyro_z / gyro_sensitivity_;

    // eventually we'll get that mag data
    // insert getting mag data here

    if (calibrated_)
    {
        // gyro calibration
        gyro_x = g_scale_ * (gyro_x - G_offset_[0]);
        gyro_y = g_scale_ * (gyro_y - G_offset_[1]);
        gyro_z = g_scale_ * (gyro_z - G_offset_[2]);

        // accel calibrations
        float tmp_x = accel_x - A_B_[0];
        float tmp_y = accel_y - A_B_[1];
        float tmp_z = accel_z - A_B_[2];

        accel_x = A_Ainv_[0][0] * tmp_x + A_Ainv_[0][1] * tmp_y + A_Ainv_[0][2] * tmp_z;
        accel_y = A_Ainv_[1][0] * tmp_x + A_Ainv_[1][1] * tmp_y + A_Ainv_[1][2] * tmp_z;
        accel_z = A_Ainv_[2][0] * tmp_x + A_Ainv_[2][1] * tmp_y + A_Ainv_[2][2] * tmp_z;

        // mag calibration
        // tmp_x = mag_x - M_B_[0];
        // tmp_y = mag_y - M_B_[1];
        // tmp_z = mag_z - M_B_[2];

        // mag_x = M_Ainv_[0][0] * tmp_x + M_Ainv_[0][1] * tmp_y + M_Ainv_[0][2] * tmp_z;
        // mag_y = M_Ainv_[1][0] * tmp_x + M_Ainv_[1][1] * tmp_y + M_Ainv_[1][2] * tmp_z;
        // mag_z = M_Ainv_[2][0] * tmp_x + M_Ainv_[2][1] * tmp_y + M_Ainv_[2][2] * tmp_z;
    }

    result.value.data.imu.accel = [ accel_x, accel_y, accel_z ];
    result.value.data.imu.gyro = [ gyro_x, gyro_y, gyro_z ];
    result.value.data.imu.mag = [ 0, 0, 0 ];

    result.status = SENSOR_OK; // this is a placeholder before we add error checking to the reads

    return result;
}