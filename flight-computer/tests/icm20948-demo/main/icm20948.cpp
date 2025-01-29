#include <stdio.h>
#include <math.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include <sys/time.h>
#include "esp_system.h"
#include "esp_log.h"

#include "icm20948.h"

static const char *TAG = "icm test";

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                                    \
    (byte & 0x80 ? '1' : '0'), (byte & 0x40 ? '1' : '0'), (byte & 0x20 ? '1' : '0'), (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), (byte & 0x04 ? '1' : '0'), (byte & 0x02 ? '1' : '0'), (byte & 0x01 ? '1' : '0')

#define ALPHA 0.99f             // Weight of gyroscope
#define RAD_TO_DEG 57.27272727f // Radians to degrees

/* ICM20948 registers */
#define ICM20948_GYRO_CONFIG_1 (0x01)
#define ICM20948_ACCEL_CONFIG (0x14)
#define ICM20948_USER_CTRL (0x03)
#define ICM20948_INT_ENABLE (0x10)
#define ICM20948_INT_ENABLE_1 (0x11)
#define ICM20948_INT_ENABLE_2 (0x12)
#define ICM20948_INT_ENABLE_3 (0x13)
#define ICM20948_INT_STATUS (0x19)
#define ICM20948_INT_STATUS_1 (0x1A)
#define ICM20948_INT_STATUS_2 (0x1B)
#define ICM20948_INT_STATUS_3 (0x1C)
#define ICM20948_ACCEL_XOUT_H (0x2D)
#define ICM20948_GYRO_XOUT_H (0x33)
#define ICM20948_TEMP_XOUT_H (0x39)
#define ICM20948_PWR_MGMT_1 (0x06)
#define ICM20948_WHO_AM_I (0x00)
#define ICM20948_REG_BANK_SEL (0x7F)

/* ICM20948 masks */
#define REG_BANK_MASK (0x30)
#define FULLSCALE_SET_MASK (0x39)
#define FULLSCALE_GET_MASK (0x06)
#define DLPF_SET_MASK (0xC7)
#define DLPF_ENABLE_MASK (0x07)
#define DLPF_DISABLE_MASK (0xFE)

/* AK09916 registers */
#define AK09916_CNTL3 (0x32)
#define AK09916_CNTL2 (0x31)
#define AK09916_MAG_XOUT_H (0x11)

#define MASTER_TRANSMIT_TIMEOUT (50)

void icm20948_write(i2c_master_dev_handle_t sensor,
                    uint8_t const *data_buf, const uint8_t data_len)
{
    ESP_ERROR_CHECK(i2c_master_transmit(sensor, data_buf, data_len, MASTER_TRANSMIT_TIMEOUT));
}

void icm20948_read(i2c_master_dev_handle_t sensor, const uint8_t reg_start_addr, uint8_t *rx, uint8_t rx_size)
{
    const uint8_t tx[] = {reg_start_addr};

    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensor, tx, sizeof(tx), rx, rx_size, MASTER_TRANSMIT_TIMEOUT));
}

ICM20948::ICM20948(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t icm20948_address, uint32_t scl_clk_speed)
    : icm20948_addr_len(addr_len), icm20948_addr(icm20948_address), icm20948_dev_handle(nullptr), ak09916_dev_handle(nullptr)
{
    // making the i2c device controlled by master
    i2c_device_config_t icm20948_cfg = {
        .dev_addr_length = addr_len,
        .device_address = icm20948_address,
        .scl_speed_hz = scl_clk_speed,
    };

    // grab the i2c bus given port and connect that shii
    i2c_master_bus_handle_t bus_handle;
    // TODO: add logging to the statements below
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(port, &bus_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &icm20948_cfg, &icm20948_dev_handle));
}

ICM20948::~ICM20948()
{
    i2c_master_bus_rm_device(icm20948_dev_handle);
    if (ak09916_dev_handle != nullptr)
    {
        i2c_master_bus_rm_device(ak09916_dev_handle);
    }
}

// copy contructor that was abandoned
// ICM20948::ICM20948(ICM20948 &&other) noexcept
//     : dev_addr_len(other.dev_addr_len),
//       dev_addr(other.dev_addr),
//       dev_handle(other.dev_handle),
//       initialized(other.initialized)
// {
//     // transfer ownership of dev handle
//     other.dev_handle = NULL;
//     other.initialized = false;
// }

void ICM20948::initAK09916(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t ak09916_address, uint32_t scl_clk_speed)
{
    activateBypassMode();
    // add a check to ensure that the BYPASS_EN bit is 0
    // maybe add a delay
    ak09916_addr_len = addr_len;
    ak09916_addr = ak09916_address;
    // making the i2c device controlled by master
    i2c_device_config_t ak09916_cfg = {
        .dev_addr_length = addr_len,
        .device_address = ak09916_address,
        .scl_speed_hz = scl_clk_speed,
    };

    // grab the i2c bus given port and connect that shii
    i2c_master_bus_handle_t bus_handle;
    // TODO: add logging to the statements below
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(port, &bus_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ak09916_cfg, &ak09916_dev_handle));
}

void ICM20948::setBank(uint8_t bank)
{
    assert(bank <= 3 && "Bank index out of range");
    bank = (bank << 4) & REG_BANK_MASK;

    const uint8_t reg_and_data[] = {ICM20948_REG_BANK_SEL, bank};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::configureICM20948(icm20948_accel_fs_t acce_fs, icm20948_gyro_fs_t gyro_fs)
{
    reset();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    wakeup();
    setBank(0);
    uint8_t dev_id = getICM20948ID();
    // TODO: add some sort of check for the device id
    // if (dev_id != ICM20948_WHO_AM_I_VAL)

    setGyroFS(gyro_fs);
    setAccelFS(acce_fs);

    setGyroSensitivity();
    setAccelSensitivity();
}

void ICM20948::configureAK09916(ak09916_sample_rate_t sample_rate)
{
    // reset the mag
    const uint8_t reg_and_data[] = {AK09916_CNTL3, 0x01};
    icm20948_write(ak09916_dev_handle, reg_and_data, sizeof(reg_and_data));

    vTaskDelay(10 / portTICK_PERIOD_MS);
    setMagSampleRate(sample_rate);
}

uint8_t
ICM20948::getICM20948ID()
{
    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_WHO_AM_I, tmp, sizeof(tmp));
    return tmp[0];
}

void ICM20948::wakeup()
{
    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_PWR_MGMT_1, tmp, sizeof(tmp));
    tmp[0] &= (~BIT6); // esp_bit_defs.h

    const uint8_t reg_and_data[] = {ICM20948_PWR_MGMT_1, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::sleep()
{
    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_PWR_MGMT_1, tmp, sizeof(tmp));
    tmp[0] |= BIT6;

    const uint8_t reg_and_data[] = {ICM20948_PWR_MGMT_1, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::reset()
{
    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_PWR_MGMT_1, tmp, sizeof(tmp));
    tmp[0] |= BIT7; // esp_bit_defs.h

    const uint8_t reg_and_data[] = {ICM20948_PWR_MGMT_1, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::setGyroFS(icm20948_gyro_fs_t gyro_fs)
{
    setBank(2);

    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_GYRO_CONFIG_1, tmp, sizeof(tmp));

#if CONFIG_LOG_DEFAULT_LEVEL == 4
    printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

    tmp[0] &= FULLSCALE_SET_MASK;
    tmp[0] |= (gyro_fs << 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
    printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

    const uint8_t reg_and_data[] = {ICM20948_GYRO_CONFIG_1, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

ICM20948::icm20948_gyro_fs_t
ICM20948::getGyroFS()
{
    setBank(2);

    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_GYRO_CONFIG_1, tmp, sizeof(tmp));

#if CONFIG_LOG_DEFAULT_LEVEL == 4
    printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

    tmp[0] &= FULLSCALE_GET_MASK;
    tmp[0] >>= 1;
    icm20948_gyro_fs_t gyro_fs = static_cast<icm20948_gyro_fs_t>(tmp[0]);
    return gyro_fs;
}

void ICM20948::setGyroSensitivity()
{
    icm20948_gyro_fs_t gyro_fs = getGyroFS();

    switch (gyro_fs)
    {
    case GYRO_FS_250DPS:
        gyro_sensitivity = 131;
        break;
    case GYRO_FS_500DPS:
        gyro_sensitivity = 65.5;
        break;
    case GYRO_FS_1000DPS:
        gyro_sensitivity = 32.8;
        break;
    case GYRO_FS_2000DPS:
        gyro_sensitivity = 16.4;
        break;
    default:
        break;
    }
}

void ICM20948::getRawGyro()
{
    uint8_t data_rd[6] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_GYRO_XOUT_H, data_rd, sizeof(data_rd));

    curr_raw_gyro_vals.raw_gyro_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    curr_raw_gyro_vals.raw_gyro_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    curr_raw_gyro_vals.raw_gyro_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));
}

void ICM20948::getGyro(icm20948_gyro_value_t *gyro_vals)
{
    setBank(0);
    getRawGyro();

    gyro_vals->gyro_x = curr_raw_gyro_vals.raw_gyro_x / gyro_sensitivity;
    gyro_vals->gyro_y = curr_raw_gyro_vals.raw_gyro_y / gyro_sensitivity;
    gyro_vals->gyro_z = curr_raw_gyro_vals.raw_gyro_z / gyro_sensitivity;
}

void ICM20948::setAccelFS(icm20948_accel_fs_t acce_fs)
{

    setBank(2);
    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_ACCEL_CONFIG, tmp, sizeof(tmp));

#if CONFIG_LOG_DEFAULT_LEVEL == 4
    printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

    tmp[0] &= FULLSCALE_SET_MASK;
    tmp[0] |= (acce_fs << 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
    printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

    const uint8_t reg_and_data[] = {ICM20948_ACCEL_CONFIG, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

ICM20948::icm20948_accel_fs_t
ICM20948::getAccelFS()
{
    setBank(2);

    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_ACCEL_CONFIG, tmp, sizeof(tmp));

#if CONFIG_LOG_DEFAULT_LEVEL == 4
    printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

    tmp[0] &= FULLSCALE_GET_MASK;
    tmp[0] >>= 1;
    icm20948_accel_fs_t acce_fs = static_cast<icm20948_accel_fs_t>(tmp[0]);
    return acce_fs;
}

void ICM20948::getRawAccel()
{
    uint8_t data_rd[6] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_ACCEL_XOUT_H, data_rd, sizeof(data_rd));

    curr_raw_accel_vals.raw_accel_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    curr_raw_accel_vals.raw_accel_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    curr_raw_accel_vals.raw_accel_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));
}

void ICM20948::setAccelSensitivity()
{
    icm20948_accel_fs_t accel_fs = getAccelFS();

    switch (accel_fs)
    {
    case ACCEL_FS_2G:
        accel_sensitivity = 16384;
        break;
    case ACCEL_FS_4G:
        accel_sensitivity = 8192;
        break;
    case ACCEL_FS_8G:
        accel_sensitivity = 4096;
        break;
    case ACCEL_FS_16G:
        accel_sensitivity = 2048;
        break;
    default:
        break;
    }
}

void ICM20948::getAccel(icm20948_accel_value_t *accel_vals)
{
    setBank(0);
    getRawAccel();

    accel_vals->accel_x = curr_raw_accel_vals.raw_accel_x / accel_sensitivity;
    accel_vals->accel_y = curr_raw_accel_vals.raw_accel_y / accel_sensitivity;
    accel_vals->accel_z = curr_raw_accel_vals.raw_accel_z / accel_sensitivity;
}

void ICM20948::activateBypassMode()
{
    setBank(0);

    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_USER_CTRL, tmp, sizeof(tmp));

    tmp[0] |= BIT5;

    const uint8_t reg_and_data[] = {ICM20948_USER_CTRL, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::setMagSampleRate(ak09916_sample_rate_t rate)
{
    // after resetting, cntl2 register should be clear with 0
    // so no need to read whats already there

    uint8_t sample_rate_setting = (rate << 1);
    const uint8_t reg_and_data[] = {AK09916_CNTL2, sample_rate_setting};
    icm20948_write(ak09916_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::getMag(ak09916_mag_value_t *mag_vals)
{
    uint8_t data_rd[6] = {0};
    icm20948_read(ak09916_dev_handle, AK09916_MAG_XOUT_H, data_rd, sizeof(data_rd));

    mag_vals->mag_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    mag_vals->mag_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    mag_vals->mag_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));
}

void ICM20948::setAccelDLPF(icm20948_dlpf_t dlpf_accel)
{
    setBank(2);

    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_ACCEL_CONFIG, tmp, sizeof(tmp));

    tmp[0] &= DLPF_SET_MASK;
    tmp[0] |= dlpf_accel << 3;

    const uint8_t reg_and_data[] = {ICM20948_ACCEL_CONFIG, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::setGyroDLPF(icm20948_dlpf_t dlpf_gyro)
{
    setBank(2);

    uint8_t tmp[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_GYRO_CONFIG_1, tmp, sizeof(tmp));

    tmp[0] &= DLPF_SET_MASK;
    tmp[0] |= dlpf_gyro << 3;

    const uint8_t reg_and_data[] = {ICM20948_GYRO_CONFIG_1, tmp[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data, sizeof(reg_and_data));
}

void ICM20948::enableDLPF(bool enable)
{
    setBank(2);

    uint8_t tmp1[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_ACCEL_CONFIG, tmp1, sizeof(tmp1));

    if (enable)
        tmp1[0] |= DLPF_ENABLE_MASK;
    else
        tmp1[0] &= DLPF_DISABLE_MASK;

    const uint8_t reg_and_data1[] = {ICM20948_ACCEL_CONFIG, tmp1[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data1, sizeof(reg_and_data1));

    uint8_t tmp2[1] = {0};
    icm20948_read(icm20948_dev_handle, ICM20948_GYRO_CONFIG_1, tmp2, sizeof(tmp2));

    if (enable)
        tmp2[0] |= DLPF_ENABLE_MASK;
    else
        tmp2[0] &= DLPF_DISABLE_MASK;

    const uint8_t reg_and_data2[] = {ICM20948_GYRO_CONFIG_1, tmp2[0]};
    icm20948_write(icm20948_dev_handle, reg_and_data2, sizeof(reg_and_data2));
}