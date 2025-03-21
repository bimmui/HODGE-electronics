#include "./i2c_ex.h"
#include "adxl375.h"

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

ADXL375::ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                 uint16_t adxl375_address, uint32_t scl_clk_speed) : config_{0, 0, false, 0x0A, 0x08, 0, 0x0B, 0}
{
    this->adxl375_dev_handle = i2c_create_device(port, addr_len, adxl375_address, scl_clk_speed);
}

ADXL375::ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                 uint16_t adxl375_address, uint32_t scl_clk_speed,
                 const ADXL375Config &cfg) : config_(cfg)
{
    this->adxl375_dev_handle = i2c_create_device(port, addr_len, adxl375_address, scl_clk_speed);
}

ADXL375::~ADXL375()
{
    i2c_remove_device(adxl375_dev_handle);
}

uint8_t ADXL375::getDevID()
{
    uint8_t tmp[1] = {0};
    _read(adxl375_dev_handle, ADXL375_WHO_AM_I_VAL, tmp, sizeof(tmp));
    return tmp[0];
}

// ApoSensor interface implementation
SensorType ADXL375::getType() const
{
    return ACCELEROMETER;
}

void ADXL375::configure()
{
    // not elegant but things get compolicated if you make i2c_write infer what the module wants
    const uint8_t reg_and_data[] = {ADXL375_ACTIVITY_INACTIVITY_CTL, config_.activity_inactivity_cntl};
    i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));

    const uint8_t reg_and_data1[] = {ADXL375_SHOCK_DETECTION_AXES_ENABLE, config_.enable_shock_detection};
    i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));

    if (config_.enable_low_power)
    {
        uint8_t reg_res = BIT4 | config_.bw_output_rate;
        const uint8_t reg_and_data2[] = {ADXL375_BW_RATE, reg_res};
        i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));
    }
    else
    {
        const uint8_t reg_and_data2[] = {ADXL375_BW_RATE, config_.bw_output_rate};
        i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));
    }

    const uint8_t reg_and_data3[] = {ADXL375_POWER_CTL, config_.power_cntl};
    i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));

    const uint8_t reg_and_data4[] = {ADXL375_ENABLE_INTERRUPTS, config_.enable_interrupts};
    i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));

    const uint8_t reg_and_data5[] = {ADXL375_DATA_FORMAT, config_.data_format};
    i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));

    const uint8_t reg_and_data6[] = {ADXL375_FIFO_CTL, config_.fifo_cntl};
    i2c_write(adxl375_dev_handle, reg_and_data, sizeof(reg_and_data));
}

bool ADXL375::initialize(void *cfg)
{
    if (getDevID() != ADXL375_WHO_AM_I_VAL)
        return false;

    configure(nullptr); // applies default config

    return true;
}

sensor_reading ADXL375::read()
{
    sensor_reading result;
    result.value.type = ACCELEROMETER;

    uint8_t data_rd[6] = {0};
    i2c_read(adxl375_dev_handle, ADXL375_ACCEL_X, data_rd, sizeof(data_rd));

    int16_t accel_x = ((int16_t)((data_rd[1] << 8) + (data_rd[0]))) * ADXL375_MG2G_MULTIPLIER;
    int16_t accel_y = ((int16_t)((data_rd[3] << 8) + (data_rd[2]))) * ADXL375_MG2G_MULTIPLIER;
    int16_t accel_z = ((int16_t)((data_rd[5] << 8) + (data_rd[4]))) * ADXL375_MG2G_MULTIPLIER;

    result.value.data.accelerometerHG.accel[0] = accel_x;
    result.value.data.accelerometerHG.accel[0] = accel_y;
    result.value.data.accelerometerHG.accel[0] = accel_z;

    result.timestamp = esp_timer_get_time() / 1000ULL; // TODO: make this a macro for microseconds

    result.status = SENSOR_OK; // this is a placeholder before we add error checking to the reads

    return result;
}