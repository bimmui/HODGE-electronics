#include "bmp581.h"
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <math.h>

#define SEA_LEVEL_PRESSURE 99500.0
#define BMP5_WHO_AM_I_VAL (0x50)

/* Timeout Defines*/
#define POWER_UP_DELAY_MS 20
#define SOFT_RESET_DELAY_MS 5

/* BMP581 Registers*/
#define BMP5_WHO_AM_I_REG (0x01)
#define BMP5_TEMP_DATA_XLSB_REG (0x1D)
#define BMP5_TEMP_DATA_LSB_REG (0x1E) // Start register for temperature
#define BMP5_INT_STATUS_REG (0x27)
#define BMP5_INT_ASSERTED_POR_SOFTRESET_COMPLETE 0x10
#define BMP5_HEALTH_STATUS_REG (0x28)
#define BMP5_OSR_CONFIG_REG (0x36)
#define BMP5_ODR_CONFIG_REG (0x37)
#define BMP5_CMD_REG (0x7E)

/* BMP581 Command defines */
#define SOFT_RESET_CMD 0xB6

/* BMP581 Configurations defines */
#define ODR_CONTINUOUS_MODE_BITS 0x03
#define RECONFIG_DELAY_MS 3

double convert_to_altitude(uint32_t raw_press)
{
    return 44330.0 * (1.0 - pow((double)raw_press / (double)SEA_LEVEL_PRESSURE, 0.1903));
}

float convert_raw_to_celsius(uint32_t raw_temperature)
{
    return raw_temperature / 65536.0; // Scaling factor called out in sheet
}

float convert_raw_to_fahrenheit(uint32_t raw_temperature)
{
    float celsius = convert_raw_to_celsius(raw_temperature);
    return (celsius * 9.0 / 5.0) + 32.0;
}
BMP581::BMP581(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
               uint16_t bmp581_address, uint32_t scl_clk_speed)
{
    this->bmp581_dev_handle_ = i2c_create_device(port, addr_len, bmp581_address, scl_clk_speed);
}

BMP581::~BMP581()
{
    i2c_remove_device(bmp581_dev_handle_);
}

uint8_t BMP581::getDevID(void)
{
    uint8_t tmp[1] = {0};
    i2c_read(bmp581_dev_handle_, BMP5_WHO_AM_I_REG, tmp, sizeof(tmp));
    return tmp[0];
}

sensor_type BMP581::getType() const
{
    return BMP;
}

void BMP581::configure()
{
    // configure the sensor to register both temp and pressure
    uint8_t curr_config[1] = {0};
    i2c_read(bmp581_dev_handle_, BMP5_OSR_CONFIG_REG, curr_config, sizeof(curr_config));

    // flip 6th bit to enable pressure readings
    curr_config[0] |= BIT6;
    uint8_t reg_and_data[2] = {BMP5_OSR_CONFIG_REG, curr_config[0]};
    i2c_write(bmp581_dev_handle_, reg_and_data, sizeof(reg_and_data));

    // put device in continous mode
    curr_config[0] = 0;
    i2c_read(bmp581_dev_handle_, BMP5_ODR_CONFIG_REG, curr_config, sizeof(curr_config));

    // We now enable the bit for continous mode
    curr_config[0] |= ODR_CONTINUOUS_MODE_BITS;
    reg_and_data[0] = BMP5_ODR_CONFIG_REG;
    reg_and_data[1] = curr_config[0];
    i2c_write(bmp581_dev_handle_, reg_and_data, sizeof(reg_and_data));

    // TODO: change this to use a macro
    vTaskDelay(pdMS_TO_TICKS(RECONFIG_DELAY_MS));
}

// TODO: add some check that ensures the soft reset was successful
sensor_status BMP581::softReset()
{
    uint8_t reset_data[2] = {BMP5_CMD_REG, SOFT_RESET_CMD};
    i2c_write(bmp581_dev_handle_, reset_data, sizeof(reset_data));
    vTaskDelay(pdMS_TO_TICKS(SOFT_RESET_DELAY_MS));

    return SENSOR_OK;
}

sensor_status BMP581::checkHealth()
{
    // TODO: double check that this is actuallly working with the if not equals 1 check
    // checking digital core/processing unit of the sensor
    // shit is bricked if it's dead
    uint8_t health_data[1] = {0};
    i2c_read(bmp581_dev_handle_, BMP5_HEALTH_STATUS_REG, health_data, sizeof(health_data));

    uint8_t core_check = health_data[0] & BIT0;
    if (core_check != 1)
        return SENSOR_ERR_BAD_HEALTH;

    // also making use of the sensor's built-in crack check
    // its like a self-test for physical/internal integrity
    uint8_t crack_check = health_data[0] & BIT7;
    if (crack_check != 1)
        return SENSOR_ERR_BAD_HEALTH;

    return SENSOR_OK;
}

sensor_status BMP581::powerUpCheck()
{

    // dummy read to initalize the sensor's interface
    uint8_t dummy_reg[1] = {0}; // A valid register address
    i2c_write(bmp581_dev_handle_, dummy_reg, sizeof(dummy_reg));

    // check chip status to make sure we successfully init
    if (getDevID() != BMP5_WHO_AM_I_VAL)
        return false;

    // check status_nvm_rdy == 1 and status_nvm_err == 0
    dummy_reg[0] = 0; // resuing this
    i2c_read(bmp581_dev_handle_, BMP5_HEALTH_STATUS_REG, dummy_reg, sizeof(dummy_reg));

    uint8_t nvm_ready = dummy_reg[0] & BIT1;
    if (nvm_ready != 1)
        return SENSOR_ERR_INIT;

    uint8_t nvm_error = dummy_reg[0] & BIT2;
    if (nvm_error != 1)
        return SENSOR_ERR_INIT;

    // int status register is cleared by reading
    dummy_reg[0] = 0; // resuing this
    i2c_read(bmp581_dev_handle_, BMP5_INT_STATUS_REG, dummy_reg, sizeof(dummy_reg));

    // ask for clean sample from the same register
    i2c_write(bmp581_dev_handle_, dummy_reg, sizeof(dummy_reg));
    i2c_read(bmp581_dev_handle_, BMP5_INT_STATUS_REG, dummy_reg, sizeof(dummy_reg));

    uint8_t por = dummy_reg[0] & BIT4;
    if (por != 1)
        return SENSOR_ERR_INIT;

    return SENSOR_OK;
}

sensor_status BMP581::initialize()
{
    sensor_status ret;
    ret = softReset();
    if (ret != SENSOR_OK)
        return ret;

    ret = powerUpCheck();
    if (ret != SENSOR_OK)
        return ret;

    ret = checkHealth(); // just an additional check to make sure sensor is ok
    if (ret != SENSOR_OK)
        return ret;

    configure();

    return ret;
}

static void BMP581::vreadTask(void *pvParameters)
{
    while (true)
    {
        sensor_reading curr_reading = read();
        ts_sensor_data *data = static_cast<ts_sensor_data *>(pvParameters);

        if (curr_reading.value.type != BMP)
            continue;

        if (curr_reading.status == SENSOR_OK)
        {
            data->baro_altitude.store(curr_reading.value.data.bmp.altitude, std::memory_order_relaxed);
            data->baro_temp.store(curr_reading.value.data.bmp.temp, std::memory_order_relaxed);
            data->baro_pressure.store(curr_reading.value.data.bmp.pressure, std::memory_order_relaxed);
        }
    }
}

sensor_reading BMP581::read()
{
    sensor_reading result;
    result.value.type = BMP;

    uint8_t data[6] = {0};
    i2c_read(bmp581_dev_handle_, BMP5_TEMP_DATA_LSB_REG, data, sizeof(data));

    // Extract raw temperature
    int32_t raw_temperature = (int32_t)((((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0]) << 8) >> 8;

    // Convert raw temperature to Celsius
    float temperature_c = convert_raw_to_celsius(raw_temperature);

    // Extract raw pressure
    uint32_t raw_pressure = ((uint32_t)data[5] << 16) | ((uint32_t)data[4] << 8) | data[3];
    float pressure = raw_pressure / 64.0;

    result.value.data.bmp.pressure = pressure;
    result.value.data.bmp.temp = temperature_c;
    result.value.data.bmp.altitude = convert_to_altitude(pressure);
    // TODO: add timestamp

    return result;
}
