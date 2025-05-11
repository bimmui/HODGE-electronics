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

#include "tmp1075.h"

/** TMP1075 Register Addresses */
#define TMP1075_TEMP_REG 0x00   // Temperature register
#define TMP1075_CONFIG_REG 0x01 // Configuration register
#define TMP1075_LOW_REG 0x02    // Low temperature threshold register
#define TMP1075_HIGH_REG 0x03   // High temperature threshold register
#define TMP1075_DEVID_REG 0x0F

/* TMP1075 masks */

/* this config mask sets the following
- does not put it in one shot mode
- 250 ms conversion rate
- 2 consecutive faults to trigger alert function
- polarity of alert pin set to low
- sets alert pin function to comparator mode
- puts the thing in continuous conversion */
// #define TMP1075_CONFIG_MASK ((uint16_t)0x6800)

void _write(i2c_master_dev_handle_t sensor,
            uint8_t const *data_buf, const uint8_t data_len)
{
    ESP_ERROR_CHECK(i2c_master_transmit(sensor, data_buf, data_len, -1));
}

void _read(i2c_master_dev_handle_t sensor, const uint8_t reg_start_addr, uint8_t *rx, uint8_t rx_size)
{
    const uint8_t tx[] = {reg_start_addr};

    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensor, tx, sizeof(tx), rx, rx_size, -1));
}

TMP1075::TMP1075(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t tmp1075_address, uint32_t scl_clk_speed)
    : tmp1075_addr_len(addr_len), tmp1075_addr(tmp1075_address), tmp1075_dev_handle(nullptr)
{
    // making the i2c device controlled by master
    i2c_device_config_t tmp1075_cfg = {
        .dev_addr_length = addr_len,
        .device_address = tmp1075_address,
        .scl_speed_hz = scl_clk_speed,
    };

    // grab the i2c bus given port and connect that shii
    i2c_master_bus_handle_t bus_handle;
    // TODO: add logging to the statements below
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(port, &bus_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &tmp1075_cfg, &tmp1075_dev_handle));
}

TMP1075::~TMP1075()
{
    i2c_master_bus_rm_device(tmp1075_dev_handle);
}

uint8_t
TMP1075::getTMP1075ID()
{
    uint8_t tmp[1] = {0};
    _read(tmp1075_dev_handle, TMP1075_DEVID_REG, tmp, sizeof(tmp));
    return tmp[0];
}

void TMP1075::configureTMP1075()
{
    // call getTMP1075ID and check if its correct

    // not gonna bother setting up the high limit nor the
    // low limit registers bc the alert pin isnt even connected

    // not using activity nor inactivity control
    uint8_t lsb = 0;
    uint8_t msb = 0x40;

    const uint8_t reg_and_data[] = {TMP1075_CONFIG_REG, lsb, msb};
    _write(tmp1075_dev_handle, reg_and_data, sizeof(reg_and_data));
}

float TMP1075::readTempC()
{
    uint8_t tmp[2] = {0};
    _read(tmp1075_dev_handle, TMP1075_TEMP_REG, tmp, sizeof(tmp));
    float tempC = ((tmp[0] << 4) + (tmp[1] >> 4)) * 0.0625;
    return tempC;
}
float TMP1075::readTempF()
{
    return ((readTempC() * 1.8f) + 32);
}