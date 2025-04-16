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

#include "adxl375.h"

/* ICM20948 registers */
#define ADXL375_POWER_CTL (0X2D)
#define ADXL375_ACCEL_X (0x32)
#define ADXL375_ACCEL_Y (0x34)
#define ADXL375_ACCEL_Z (0x36)
#define ADXL375_ACTIVITY_INACTIVITY_CTL (0x27)
#define ADXL375_SHOCK_DETECTION_AXES_ENABLE (0x2A)
#define ADXL375_BW_RATE (0x2C)
#define ADXL375_ENABLE_INTERRUPTS (0x2E)
#define ADXL375_DATA_FORMAT (0x31)
#define ADXL375_FIFO_CTL (0x38)

#define ADXL375_WRITE_ADDR (0xA6)
#define ADXL375_READ_ADDR (0xA7)

void adxl375_write(i2c_master_dev_handle_t sensor,
                   uint8_t const *data_buf, const uint8_t data_len)
{
    ESP_ERROR_CHECK(i2c_master_transmit(sensor, data_buf, data_len, 50));
}

void adxl375_read(i2c_master_dev_handle_t sensor, const uint8_t reg_start_addr, uint8_t *rx, uint8_t rx_size)
{
    const uint8_t tx[] = {reg_start_addr};

    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensor, tx, sizeof(tx), rx, rx_size, 50));
}

ADXL375::ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t adxl375_address, uint32_t scl_clk_speed)
    : adxl375_addr_len(addr_len), adxl375_addr(adxl375_address), adxl375_dev_handle(nullptr)
{
    // making the i2c device controlled by master
    i2c_device_config_t adxl375_cfg = {
        .dev_addr_length = addr_len,
        .device_address = adxl375_address,
        .scl_speed_hz = scl_clk_speed,
    };

    // grab the i2c bus given port and connect that shii
    i2c_master_bus_handle_t bus_handle;
    // TODO: add logging to the statements below
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(port, &bus_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &adxl375_cfg, &adxl375_dev_handle));
}

ADXL375::~ADXL375()
{
    i2c_master_bus_rm_device(adxl375_dev_handle);
}

uint8_t
ADXL375::getADXL375ID()
{
    uint8_t tmp[1] = {0};
    adxl375_read(adxl375_dev_handle, ADXL375_WHO_AM_I_VAL, tmp, sizeof(tmp));
    return tmp[0];
}

void ADXL375::configureADXL375()
{

    // call getADXL375ID and check if its correct

    // not using activity nor inactivity control
    const uint8_t reg_and_data[] = {ADXL375_ACTIVITY_INACTIVITY_CTL, 0};
    adxl375_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));

    // not using shock detection either, also gonna reuse the reg_and_data arr
    const uint8_t reg_and_data1[] = {ADXL375_SHOCK_DETECTION_AXES_ENABLE, 0};
    adxl375_write(adxl375_dev_handle, reg_and_data1, sizeof(reg_and_data1));

    // fixing the device bandwidth length and output data rate to 100 Hz
    // not letting it be in low power mode
    const uint8_t reg_and_data2[] = {ADXL375_BW_RATE, 0x0A};
    adxl375_write(adxl375_dev_handle, reg_and_data2, sizeof(reg_and_data2));

    // setting the device to measure mode
    const uint8_t reg_and_data3[] = {ADXL375_POWER_CTL, 0x08};
    adxl375_write(adxl375_dev_handle, reg_and_data3, sizeof(reg_and_data3));

    // not using interrupts
    const uint8_t reg_and_data4[] = {ADXL375_ENABLE_INTERRUPTS, 0};
    adxl375_write(adxl375_dev_handle, reg_and_data4, sizeof(reg_and_data4));

    // making the data right justified (LSB)
    const uint8_t reg_and_data5[] = {ADXL375_DATA_FORMAT, 0x0B};
    adxl375_write(adxl375_dev_handle, reg_and_data5, sizeof(reg_and_data5));

    // not using the fifo either
    const uint8_t reg_and_data6[] = {ADXL375_FIFO_CTL, 0};
    adxl375_write(adxl375_dev_handle, reg_and_data6, sizeof(reg_and_data6));
}

void ADXL375::getAccel(adxl375_accel_value_t *accel_vals)
{
    uint8_t data_rd[6] = {0};
    adxl375_read(adxl375_dev_handle, ADXL375_ACCEL_X, data_rd, sizeof(data_rd));

    accel_vals->accel_x = ((int16_t)((data_rd[1] << 8) + (data_rd[0]))) * ADXL375_MG2G_MULTIPLIER;
    accel_vals->accel_y = ((int16_t)((data_rd[3] << 8) + (data_rd[2]))) * ADXL375_MG2G_MULTIPLIER;
    accel_vals->accel_z = ((int16_t)((data_rd[5] << 8) + (data_rd[4]))) * ADXL375_MG2G_MULTIPLIER;
}