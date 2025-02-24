#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF includes for new I2C master API
#include "driver/i2c_master.h"

// ESP-IDF includes for SPI
#include "driver/spi_master.h"

//----------------------------------------------------------------------
// A small "unified sensor" style definitions for demonstration
//----------------------------------------------------------------------
typedef enum
{
    SENSOR_TYPE_ACCELEROMETER = 1,
    SENSOR_TYPE_MAGNETIC_FIELD = 2,
    SENSOR_TYPE_GYROSCOPE = 4,
    SENSOR_TYPE_AMBIENT_TEMPERATURE = 13,
} sensors_type_t;

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} lsm9ds1Data_t;

typedef struct
{
    uint32_t version;
    uint32_t sensor_id;
    uint32_t type;
    uint32_t timestamp;

    union
    {
        struct
        {
            float x;
            float y;
            float z;
        } acceleration;

        struct
        {
            float x;
            float y;
            float z;
        } gyro;

        struct
        {
            float x;
            float y;
            float z;
        } magnetic;

        float temperature;
    };
} sensors_event_t;

typedef struct
{
    char name[12];
    int32_t version;
    int32_t sensor_id;
    int32_t type;
    float max_value;
    float min_value;
    float resolution;
    int32_t min_delay;
} sensor_t;

//----------------------------------------------------------------------
// LSM9DS1 Registers & Values
//----------------------------------------------------------------------
#define LSM9DS1_ADDRESS_ACCELGYRO (0x6B) // or 0x6A
#define LSM9DS1_ADDRESS_MAG (0x1E)       // or 0x1C

#define LSM9DS1_XG_ID 0x68
#define LSM9DS1_MAG_ID 0x3D

// Accelerometer/Gyro registers
#define LSM9DS1_REGISTER_WHO_AM_I_XG 0x0F
#define LSM9DS1_REGISTER_CTRL_REG1_G 0x10
#define LSM9DS1_REGISTER_CTRL_REG5_XL 0x1F
#define LSM9DS1_REGISTER_CTRL_REG6_XL 0x20
#define LSM9DS1_REGISTER_CTRL_REG8 0x22
#define LSM9DS1_REGISTER_OUT_X_L_G 0x18
#define LSM9DS1_REGISTER_OUT_X_L_XL 0x28
#define LSM9DS1_REGISTER_TEMP_OUT_L 0x15

// Magnetometer registers
#define LSM9DS1_REGISTER_WHO_AM_I_M 0x0F
#define LSM9DS1_REGISTER_CTRL_REG2_M 0x21
#define LSM9DS1_REGISTER_CTRL_REG3_M 0x22
#define LSM9DS1_REGISTER_OUT_X_L_M 0x28

// Soft reset for accel/gyro
#define LSM9DS1_CTRL_REG8_SW_RESET 0x05

// Scales and data rates (some examples)
typedef enum
{
    LSM9DS1_ACCELRANGE_2G = 0x00,
    LSM9DS1_ACCELRANGE_16G = 0x18,
    LSM9DS1_ACCELRANGE_4G = 0x10,
    LSM9DS1_ACCELRANGE_8G = 0x08
} lsm9ds1AccelRange_t;

typedef enum
{
    LSM9DS1_ACCELDATARATE_POWERDOWN = 0x00,
    LSM9DS1_ACCELDATARATE_10HZ = 0x20,
    LSM9DS1_ACCELDATARATE_50HZ = 0x40,
    LSM9DS1_ACCELDATARATE_119HZ = 0x60,
    LSM9DS1_ACCELDATARATE_238HZ = 0x80,
    LSM9DS1_ACCELDATARATE_476HZ = 0xA0,
    LSM9DS1_ACCELDATARATE_952HZ = 0xC0
} lsm9ds1AccelDataRate_t;

typedef enum
{
    LSM9DS1_MAGGAIN_4GAUSS = 0x00,  // +/- 4 gauss
    LSM9DS1_MAGGAIN_8GAUSS = 0x20,  // +/- 8 gauss
    LSM9DS1_MAGGAIN_12GAUSS = 0x40, // +/- 12 gauss
    LSM9DS1_MAGGAIN_16GAUSS = 0x60  // +/- 16 gauss
} lsm9ds1MagGain_t;

typedef enum
{
    LSM9DS1_GYROSCALE_245DPS = 0x00,
    LSM9DS1_GYROSCALE_500DPS = 0x08,
    LSM9DS1_GYROSCALE_2000DPS = 0x18
} lsm9ds1GyroScale_t;

// Sensitivity constants
#define LSM9DS1_ACCEL_MG_LSB_2G (0.061F)
#define LSM9DS1_ACCEL_MG_LSB_4G (0.122F)
#define LSM9DS1_ACCEL_MG_LSB_8G (0.244F)
#define LSM9DS1_ACCEL_MG_LSB_16G (0.732F)

#define LSM9DS1_GYRO_DPS_DIGIT_245DPS (0.00875F)
#define LSM9DS1_GYRO_DPS_DIGIT_500DPS (0.01750F)
#define LSM9DS1_GYRO_DPS_DIGIT_2000DPS (0.07000F)

// Unit conversions
#define SENSORS_DPS_TO_RADS (0.017453293F)
#define SENSORS_GRAVITY_STANDARD (9.80665F)

class LSM9DS1_ESP_IDF
{
public:
    // Constructor. For demonstration, you can pass nothing or just choose your own approach.
    LSM9DS1_ESP_IDF();

    /**
     * @brief  Initialize sensor on the new I2C Master bus API
     * @param  bus_handle        A handle returned by i2c_new_master_bus()
     * @param  addrAG            7-bit address for Accel+Gyro subchip
     * @param  addrM             7-bit address for Magnetometer subchip
     * @param  scl_speed_hz      Desired I2C bus speed (e.g. 400kHz)
     */
    esp_err_t initI2C(i2c_master_bus_handle_t bus_handle,
                      uint16_t addrAG = LSM9DS1_ADDRESS_ACCELGYRO,
                      uint16_t addrM = LSM9DS1_ADDRESS_MAG,
                      uint32_t scl_speed_hz = 400000);

    /**
     * @brief  Initialize sensor on SPI
     *         (unchanged from older examples, for completeness)
     */
    esp_err_t initSPI(spi_host_device_t host, gpio_num_t sclk, gpio_num_t miso,
                      gpio_num_t mosi, gpio_num_t csAG, gpio_num_t csM,
                      int clock_speed_hz);

    /**
     * @brief  Check IDs, do soft resets, set up basic sensor config
     */
    bool begin();

    // Read raw sensor data
    void read();
    void readAccel();
    void readGyro();
    void readMag();
    void readTemp();

    // Setup sensor ranges
    void setupAccel(lsm9ds1AccelRange_t range, lsm9ds1AccelDataRate_t rate);
    void setupGyro(lsm9ds1GyroScale_t scale);
    void setupMag(lsm9ds1MagGain_t gain);

    // Retrieve last read sensor data
    lsm9ds1Data_t accelData, gyroData, magData;
    int16_t temperature;

    // "Unified" style event
    bool getEvent(sensors_event_t *accelEvent,
                  sensors_event_t *magEvent,
                  sensors_event_t *gyroEvent,
                  sensors_event_t *tempEvent);
    void getAccelEvent(sensors_event_t *event, uint32_t timestamp);
    void getMagEvent(sensors_event_t *event, uint32_t timestamp);
    void getGyroEvent(sensors_event_t *event, uint32_t timestamp);
    void getTempEvent(sensors_event_t *event, uint32_t timestamp);

    // "Unified" style sensor descriptions
    void getSensor(sensor_t *accel, sensor_t *mag, sensor_t *gyro, sensor_t *temp);
    void getAccelSensor(sensor_t *sensor);
    void getMagSensor(sensor_t *sensor);
    void getGyroSensor(sensor_t *sensor);
    void getTempSensor(sensor_t *sensor);

private:
    // Low-level I2C routines (new driver) for AG and M
    void i2cWriteAG(const uint8_t *data, size_t len);
    void i2cReadAG(uint8_t reg, uint8_t *rx, size_t rx_size);
    void i2cWriteM(const uint8_t *data, size_t len);
    void i2cReadM(uint8_t reg, uint8_t *rx, size_t rx_size);

    // Low-level SPI routines (unchanged from older example)
    void spiWriteAG(uint8_t reg, uint8_t value);
    void spiReadBufferAG(uint8_t reg, uint8_t *buffer, uint8_t len);
    void spiWriteM(uint8_t reg, uint8_t value);
    void spiReadBufferM(uint8_t reg, uint8_t *buffer, uint8_t len);

    // Helpers
    uint8_t readRegisterAG(uint8_t reg);
    void writeRegisterAG(uint8_t reg, uint8_t value);
    uint8_t readRegisterM(uint8_t reg);
    void writeRegisterM(uint8_t reg, uint8_t value);

    void resetAndRebootAG();
    uint32_t getMillis(); // or use esp_timer_get_time() / 1000

private:
    // I2C or SPI usage
    bool _useSPI = false;

    // I2C Master
    i2c_master_bus_handle_t _i2cBusHandle = nullptr;
    i2c_master_dev_handle_t _i2cDevHandleAG = nullptr;
    i2c_master_dev_handle_t _i2cDevHandleM = nullptr;

    // SPI handles
    spi_device_handle_t _spiHandleAG = nullptr;
    spi_device_handle_t _spiHandleM = nullptr;

    // Range scaling
    float _accel_mg_lsb = LSM9DS1_ACCEL_MG_LSB_2G;
    float _gyro_dps_digit = LSM9DS1_GYRO_DPS_DIGIT_245DPS;

    // Example sensor IDs
    int32_t _sensorid_accel = 1;
    int32_t _sensorid_gyro = 2;
    int32_t _sensorid_mag = 3;
    int32_t _sensorid_temp = 4;
};
