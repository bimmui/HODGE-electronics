#include "LSM9DS1_ESP_IDF.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LSM9DS1";

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------
LSM9DS1_ESP_IDF::LSM9DS1_ESP_IDF()
{
    // default fields already set in-class
}

//----------------------------------------------------------------------
// Initialize using the new I2C Master driver
//----------------------------------------------------------------------
esp_err_t LSM9DS1_ESP_IDF::initI2C(i2c_port_num_t port,
                                   uint16_t addrAG,
                                   uint16_t addrM,
                                   uint32_t scl_speed_hz)
{
    _useSPI = false;

    // 1) Create device for Accel/Gyro
    {
        i2c_device_config_t devcfgAG = {};
        devcfgAG.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        devcfgAG.device_address = addrAG;     // 7-bit
        devcfgAG.scl_speed_hz = scl_speed_hz; // Typically 400k for LSM9DS1

        i2c_master_bus_handle_t bus_handle;
        _i2cBusHandle = bus_handle;
        // TODO: add logging to the statements below
        ESP_ERROR_CHECK(i2c_master_get_bus_handle(port, &bus_handle));
        esp_err_t ret = i2c_master_bus_add_device(_i2cBusHandle, &devcfgAG, &_i2cDevHandleAG);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to add I2C device for AG: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    // 2) Create device for Magnetometer
    {
        i2c_device_config_t devcfgM = {};
        devcfgM.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        devcfgM.device_address = addrM;
        devcfgM.scl_speed_hz = scl_speed_hz;

        esp_err_t ret = i2c_master_bus_add_device(_i2cBusHandle, &devcfgM, &_i2cDevHandleM);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to add I2C device for M: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    return ESP_OK;
}

//----------------------------------------------------------------------
// Initialize SPI (unchanged from older example, for completeness)
//----------------------------------------------------------------------
esp_err_t LSM9DS1_ESP_IDF::initSPI(spi_host_device_t host, gpio_num_t sclk, gpio_num_t miso,
                                   gpio_num_t mosi, gpio_num_t csAG, gpio_num_t csM,
                                   int clock_speed_hz)
{
    _useSPI = true;

    // Initialize the SPI bus if not already done. (User must ensure not to re-init)
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = sclk;
    buscfg.mosi_io_num = mosi;
    buscfg.miso_io_num = miso;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    esp_err_t ret = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        return ret;
    }

    // Accel/Gyro
    spi_device_interface_config_t devcfgAG = {};
    devcfgAG.clock_speed_hz = clock_speed_hz;
    devcfgAG.mode = 0;
    devcfgAG.spics_io_num = csAG;
    devcfgAG.queue_size = 1;

    ret = spi_bus_add_device(host, &devcfgAG, &_spiHandleAG);
    if (ret != ESP_OK)
        return ret;

    // Magnetometer
    spi_device_interface_config_t devcfgM = {};
    devcfgM.clock_speed_hz = clock_speed_hz;
    devcfgM.mode = 0;
    devcfgM.spics_io_num = csM;
    devcfgM.queue_size = 1;

    ret = spi_bus_add_device(host, &devcfgM, &_spiHandleM);
    return ret;
}

//----------------------------------------------------------------------
// begin() - check IDs, set defaults, etc.
//----------------------------------------------------------------------
bool LSM9DS1_ESP_IDF::begin()
{
    // Soft reset & reboot
    resetAndRebootAG();
    vTaskDelay(pdMS_TO_TICKS(10));

    // Check Accel/Gyro
    uint8_t id_xg = readRegisterAG(LSM9DS1_REGISTER_WHO_AM_I_XG);
    if (id_xg != LSM9DS1_XG_ID)
    {
        ESP_LOGE(TAG, "Accel/Gyro not found! Expected 0x%02X, got 0x%02X", LSM9DS1_XG_ID, id_xg);
        return false;
    }

    // Check Magnetometer
    uint8_t id_m = readRegisterM(LSM9DS1_REGISTER_WHO_AM_I_M);
    if (id_m != LSM9DS1_MAG_ID)
    {
        ESP_LOGE(TAG, "Mag not found! Expected 0x%02X, got 0x%02X", LSM9DS1_MAG_ID, id_m);
        return false;
    }

    // Enable Gyro continuous
    writeRegisterAG(LSM9DS1_REGISTER_CTRL_REG1_G, 0xC0);

    // Enable Accel, 1kHz data rate
    writeRegisterAG(LSM9DS1_REGISTER_CTRL_REG5_XL, 0x38);
    writeRegisterAG(LSM9DS1_REGISTER_CTRL_REG6_XL, 0xC0);

    // Magnetometer continuous mode => CTRL_REG3_M = 0x00
    writeRegisterM(LSM9DS1_REGISTER_CTRL_REG3_M, 0x00);

    // Default ranges
    setupAccel(LSM9DS1_ACCELRANGE_2G, LSM9DS1_ACCELDATARATE_10HZ);
    setupGyro(LSM9DS1_GYROSCALE_245DPS);
    setupMag(LSM9DS1_MAGGAIN_4GAUSS);

    return true;
}

//----------------------------------------------------------------------
// Read all sensors
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::read()
{
    readAccel();
    readGyro();
    readTemp();
    readMag();
}

//----------------------------------------------------------------------
// Read Accel
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::readAccel()
{
    uint8_t buffer[6] = {0};
    if (_useSPI)
    {
        spiReadBufferAG(0x80 | LSM9DS1_REGISTER_OUT_X_L_XL, buffer, 6);
    }
    else
    {
        i2cReadAG(0x80 | LSM9DS1_REGISTER_OUT_X_L_XL, buffer, 6);
    }

    int16_t x = (int16_t)((uint16_t)buffer[1] << 8 | buffer[0]);
    int16_t y = (int16_t)((uint16_t)buffer[3] << 8 | buffer[2]);
    int16_t z = (int16_t)((uint16_t)buffer[5] << 8 | buffer[4]);

    accelData.x = x;
    accelData.y = y;
    accelData.z = z;
}

//----------------------------------------------------------------------
// Read Gyro
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::readGyro()
{
    uint8_t buffer[6] = {0};
    if (_useSPI)
    {
        spiReadBufferAG(0x80 | LSM9DS1_REGISTER_OUT_X_L_G, buffer, 6);
    }
    else
    {
        i2cReadAG(0x80 | LSM9DS1_REGISTER_OUT_X_L_G, buffer, 6);
    }

    int16_t x = (int16_t)((uint16_t)buffer[1] << 8 | buffer[0]);
    int16_t y = (int16_t)((uint16_t)buffer[3] << 8 | buffer[2]);
    int16_t z = (int16_t)((uint16_t)buffer[5] << 8 | buffer[4]);

    gyroData.x = x;
    gyroData.y = y;
    gyroData.z = z;
}

//----------------------------------------------------------------------
// Read Magnetometer
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::readMag()
{
    uint8_t buffer[6] = {0};
    if (_useSPI)
    {
        spiReadBufferM(0x80 | LSM9DS1_REGISTER_OUT_X_L_M, buffer, 6);
    }
    else
    {
        i2cReadM(0x80 | LSM9DS1_REGISTER_OUT_X_L_M, buffer, 6);
    }

    int16_t x = (int16_t)((uint16_t)buffer[1] << 8 | buffer[0]);
    int16_t y = (int16_t)((uint16_t)buffer[3] << 8 | buffer[2]);
    int16_t z = (int16_t)((uint16_t)buffer[5] << 8 | buffer[4]);

    magData.x = x;
    magData.y = y;
    magData.z = z;

    // magData.x = x;
    // magData.y = y;
    // magData.z = z;
}

//----------------------------------------------------------------------
// Read Temperature
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::readTemp()
{
    uint8_t buffer[2] = {0};
    if (_useSPI)
    {
        spiReadBufferAG(0x80 | LSM9DS1_REGISTER_TEMP_OUT_L, buffer, 2);
    }
    else
    {
        i2cReadAG(0x80 | LSM9DS1_REGISTER_TEMP_OUT_L, buffer, 2);
    }

    int16_t raw = (int16_t)((uint16_t)buffer[1] << 8 | buffer[0]);
    temperature = raw;
}

//----------------------------------------------------------------------
// Set up Accelerometer
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::setupAccel(lsm9ds1AccelRange_t range,
                                 lsm9ds1AccelDataRate_t rate)
{
    uint8_t reg = readRegisterAG(LSM9DS1_REGISTER_CTRL_REG6_XL);
    reg &= ~(0xF8);
    reg |= (range | rate);
    writeRegisterAG(LSM9DS1_REGISTER_CTRL_REG6_XL, reg);

    switch (range)
    {
    case LSM9DS1_ACCELRANGE_2G:
        _accel_mg_lsb = LSM9DS1_ACCEL_MG_LSB_2G;
        break;
    case LSM9DS1_ACCELRANGE_4G:
        _accel_mg_lsb = LSM9DS1_ACCEL_MG_LSB_4G;
        break;
    case LSM9DS1_ACCELRANGE_8G:
        _accel_mg_lsb = LSM9DS1_ACCEL_MG_LSB_8G;
        break;
    case LSM9DS1_ACCELRANGE_16G:
        _accel_mg_lsb = LSM9DS1_ACCEL_MG_LSB_16G;
        break;
    }
}

//----------------------------------------------------------------------
// Set up Gyro
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::setupGyro(lsm9ds1GyroScale_t scale)
{
    uint8_t reg = readRegisterAG(LSM9DS1_REGISTER_CTRL_REG1_G);
    reg &= ~0x18;
    reg |= (uint8_t)scale;
    writeRegisterAG(LSM9DS1_REGISTER_CTRL_REG1_G, reg);

    switch (scale)
    {
    case LSM9DS1_GYROSCALE_245DPS:
        _gyro_dps_digit = LSM9DS1_GYRO_DPS_DIGIT_245DPS;
        break;
    case LSM9DS1_GYROSCALE_500DPS:
        _gyro_dps_digit = LSM9DS1_GYRO_DPS_DIGIT_500DPS;
        break;
    case LSM9DS1_GYROSCALE_2000DPS:
        _gyro_dps_digit = LSM9DS1_GYRO_DPS_DIGIT_2000DPS;
        break;
    }
}

//----------------------------------------------------------------------
// Set up Magnetometer
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::setupMag(lsm9ds1MagGain_t gain)
{
    // The gain bits are in CTRL_REG2_M [5:5]
    uint8_t reg = readRegisterM(LSM9DS1_REGISTER_CTRL_REG2_M);
    reg &= ~0x60;
    reg |= (uint8_t)gain;
    writeRegisterM(LSM9DS1_REGISTER_CTRL_REG2_M, reg);
}

//----------------------------------------------------------------------
// Unified sensor event retrieval
//----------------------------------------------------------------------
bool LSM9DS1_ESP_IDF::getEvent(sensors_event_t *accelEvent,
                               sensors_event_t *magEvent,
                               sensors_event_t *gyroEvent,
                               sensors_event_t *tempEvent)
{
    read();
    uint32_t timestamp = getMillis();

    if (accelEvent)
        getAccelEvent(accelEvent, timestamp);
    if (magEvent)
        getMagEvent(magEvent, timestamp);
    if (gyroEvent)
        getGyroEvent(gyroEvent, timestamp);
    if (tempEvent)
        getTempEvent(tempEvent, timestamp);

    return true;
}

void LSM9DS1_ESP_IDF::getAccelEvent(sensors_event_t *event, uint32_t timestamp)
{
    memset(event, 0, sizeof(*event));
    event->version = sizeof(sensors_event_t);
    event->sensor_id = _sensorid_accel;
    event->type = SENSOR_TYPE_ACCELEROMETER;
    event->timestamp = timestamp;

    // Convert raw to m/s^2
    event->acceleration.x = accelData.x * _accel_mg_lsb / 1000.0f * SENSORS_GRAVITY_STANDARD;
    event->acceleration.y = accelData.y * _accel_mg_lsb / 1000.0f * SENSORS_GRAVITY_STANDARD;
    event->acceleration.z = accelData.z * _accel_mg_lsb / 1000.0f * SENSORS_GRAVITY_STANDARD;
}

void LSM9DS1_ESP_IDF::getMagEvent(sensors_event_t *event, uint32_t timestamp)
{
    memset(event, 0, sizeof(*event));
    event->version = sizeof(sensors_event_t);
    event->sensor_id = _sensorid_mag;
    event->type = SENSOR_TYPE_MAGNETIC_FIELD;
    event->timestamp = timestamp;

    // approximate sensitivity for +/-4 gauss:
    float magSensitivity = 0.14f;

    event->magnetic.x = magData.x * magSensitivity / 10.0f;
    event->magnetic.y = magData.y * magSensitivity / 10.0f;
    event->magnetic.z = magData.z * magSensitivity / 10.0f;
}

void LSM9DS1_ESP_IDF::getGyroEvent(sensors_event_t *event, uint32_t timestamp)
{
    memset(event, 0, sizeof(*event));
    event->version = sizeof(sensors_event_t);
    event->sensor_id = _sensorid_gyro;
    event->type = SENSOR_TYPE_GYROSCOPE;
    event->timestamp = timestamp;

    // Convert raw to rad/s
    event->gyro.x = gyroData.x * _gyro_dps_digit * SENSORS_DPS_TO_RADS;
    event->gyro.y = gyroData.y * _gyro_dps_digit * SENSORS_DPS_TO_RADS;
    event->gyro.z = gyroData.z * _gyro_dps_digit * SENSORS_DPS_TO_RADS;
}

void LSM9DS1_ESP_IDF::getTempEvent(sensors_event_t *event, uint32_t timestamp)
{
    memset(event, 0, sizeof(*event));
    event->version = sizeof(sensors_event_t);
    event->sensor_id = _sensorid_temp;
    event->type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
    event->timestamp = timestamp;

    // Example formula from datasheet: 21 + (raw / 8)
    event->temperature = 21.0f + ((float)temperature / 8.0f);
}

//----------------------------------------------------------------------
// Unified sensor metadata
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::getSensor(sensor_t *accel, sensor_t *mag, sensor_t *gyro, sensor_t *temp)
{
    if (accel)
        getAccelSensor(accel);
    if (mag)
        getMagSensor(mag);
    if (gyro)
        getGyroSensor(gyro);
    if (temp)
        getTempSensor(temp);
}

void LSM9DS1_ESP_IDF::getAccelSensor(sensor_t *sensor)
{
    memset(sensor, 0, sizeof(*sensor));
    strncpy(sensor->name, "LSM9DS1_A", sizeof(sensor->name) - 1);
    sensor->version = 1;
    sensor->sensor_id = _sensorid_accel;
    sensor->type = SENSOR_TYPE_ACCELEROMETER;
    sensor->min_delay = 0;
    sensor->max_value = 16.0f * SENSORS_GRAVITY_STANDARD;
    sensor->min_value = -16.0f * SENSORS_GRAVITY_STANDARD;
    sensor->resolution = LSM9DS1_ACCEL_MG_LSB_2G / 1000.0f * SENSORS_GRAVITY_STANDARD;
}

void LSM9DS1_ESP_IDF::getMagSensor(sensor_t *sensor)
{
    memset(sensor, 0, sizeof(*sensor));
    strncpy(sensor->name, "LSM9DS1_M", sizeof(sensor->name) - 1);
    sensor->version = 1;
    sensor->sensor_id = _sensorid_mag;
    sensor->type = SENSOR_TYPE_MAGNETIC_FIELD;
    sensor->min_delay = 0;
    sensor->max_value = 16.0f;
    sensor->min_value = -16.0f;
    sensor->resolution = 0.001f;
}

void LSM9DS1_ESP_IDF::getGyroSensor(sensor_t *sensor)
{
    memset(sensor, 0, sizeof(*sensor));
    strncpy(sensor->name, "LSM9DS1_G", sizeof(sensor->name) - 1);
    sensor->version = 1;
    sensor->sensor_id = _sensorid_gyro;
    sensor->type = SENSOR_TYPE_GYROSCOPE;
    sensor->min_delay = 0;
    // 2000 DPS -> ~34.9 rad/s
    sensor->max_value = 34.9f;
    sensor->min_value = -34.9f;
    sensor->resolution = LSM9DS1_GYRO_DPS_DIGIT_245DPS * SENSORS_DPS_TO_RADS;
}

void LSM9DS1_ESP_IDF::getTempSensor(sensor_t *sensor)
{
    memset(sensor, 0, sizeof(*sensor));
    strncpy(sensor->name, "LSM9DS1_T", sizeof(sensor->name) - 1);
    sensor->version = 1;
    sensor->sensor_id = _sensorid_temp;
    sensor->type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
    sensor->min_delay = 0;
    sensor->max_value = 80.0f;
    sensor->min_value = -40.0f;
    sensor->resolution = 1.0f;
}

//----------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::resetAndRebootAG()
{
    // CTRL_REG8 = 0x05 => Reboot + SW reset
    writeRegisterAG(LSM9DS1_REGISTER_CTRL_REG8, LSM9DS1_CTRL_REG8_SW_RESET);
}

uint32_t LSM9DS1_ESP_IDF::getMillis()
{
    // Convert microseconds to milliseconds
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

//----------------------------------------------------------------------
// I2C (new driver) low-level reads/writes
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::i2cWriteAG(const uint8_t *data, size_t len)
{
    esp_err_t err = i2c_master_transmit(_i2cDevHandleAG, data, len, 50);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2cWriteAG err: %s", esp_err_to_name(err));
    }
}

void LSM9DS1_ESP_IDF::i2cReadAG(uint8_t reg, uint8_t *rx, size_t rx_size)
{
    const uint8_t tx[] = {reg};
    esp_err_t err = i2c_master_transmit_receive(_i2cDevHandleAG, tx, sizeof(tx), rx, rx_size, 50);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2cReadAG err: %s", esp_err_to_name(err));
    }
}

void LSM9DS1_ESP_IDF::i2cWriteM(const uint8_t *data, size_t len)
{
    esp_err_t err = i2c_master_transmit(_i2cDevHandleM, data, len, 50);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2cWriteM err: %s", esp_err_to_name(err));
    }
}

void LSM9DS1_ESP_IDF::i2cReadM(uint8_t reg, uint8_t *rx, size_t rx_size)
{
    const uint8_t tx[] = {reg};
    esp_err_t err = i2c_master_transmit_receive(_i2cDevHandleM, tx, sizeof(tx), rx, rx_size, 50);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2cReadM err: %s", esp_err_to_name(err));
    }
}

//----------------------------------------------------------------------
// SPI low-level reads/writes (unchanged from older example)
//----------------------------------------------------------------------
void LSM9DS1_ESP_IDF::spiWriteAG(uint8_t reg, uint8_t value)
{
    uint8_t txData[2];
    txData[0] = reg & 0x7F; // write
    txData[1] = value;

    spi_transaction_t t = {};
    t.length = 16;
    t.tx_buffer = txData;

    spi_device_transmit(_spiHandleAG, &t);
}

void LSM9DS1_ESP_IDF::spiReadBufferAG(uint8_t reg, uint8_t *buffer, uint8_t len)
{
    uint8_t txData = reg | 0x80; // read
    uint8_t rxData[32];
    memset(rxData, 0, sizeof(rxData));

    spi_transaction_t t = {};
    t.length = (len + 1) * 8;
    t.rxlength = (len + 1) * 8;
    t.tx_buffer = &txData;
    t.rx_buffer = rxData;

    spi_device_transmit(_spiHandleAG, &t);

    for (int i = 0; i < len; i++)
    {
        buffer[i] = rxData[i + 1];
    }
}

void LSM9DS1_ESP_IDF::spiWriteM(uint8_t reg, uint8_t value)
{
    uint8_t txData[2];
    txData[0] = reg & 0x7F;
    txData[1] = value;

    spi_transaction_t t = {};
    t.length = 16;
    t.tx_buffer = txData;

    spi_device_transmit(_spiHandleM, &t);
}

void LSM9DS1_ESP_IDF::spiReadBufferM(uint8_t reg, uint8_t *buffer, uint8_t len)
{
    uint8_t txData = reg | 0x80; // read
    uint8_t rxData[32];
    memset(rxData, 0, sizeof(rxData));

    spi_transaction_t t = {};
    t.length = (len + 1) * 8;
    t.rxlength = (len + 1) * 8;
    t.tx_buffer = &txData;
    t.rx_buffer = rxData;

    spi_device_transmit(_spiHandleM, &t);

    for (int i = 0; i < len; i++)
    {
        buffer[i] = rxData[i + 1];
    }
}

//----------------------------------------------------------------------
// High-level read/write "register" helpers
//----------------------------------------------------------------------
uint8_t LSM9DS1_ESP_IDF::readRegisterAG(uint8_t reg)
{
    uint8_t val = 0;
    if (_useSPI)
    {
        spiReadBufferAG(reg | 0x80, &val, 1);
    }
    else
    {
        i2cReadAG(reg, &val, 1);
    }
    return val;
}

void LSM9DS1_ESP_IDF::writeRegisterAG(uint8_t reg, uint8_t value)
{
    if (_useSPI)
    {
        spiWriteAG(reg, value);
    }
    else
    {
        uint8_t tx[] = {reg, value};
        i2cWriteAG(tx, sizeof(tx));
    }
}

uint8_t LSM9DS1_ESP_IDF::readRegisterM(uint8_t reg)
{
    uint8_t val = 0;
    if (_useSPI)
    {
        spiReadBufferM(reg | 0x80, &val, 1);
    }
    else
    {
        i2cReadM(reg, &val, 1);
    }
    return val;
}

void LSM9DS1_ESP_IDF::writeRegisterM(uint8_t reg, uint8_t value)
{
    if (_useSPI)
    {
        spiWriteM(reg, value);
    }
    else
    {
        uint8_t tx[] = {reg, value};
        i2cWriteM(tx, sizeof(tx));
    }
}
