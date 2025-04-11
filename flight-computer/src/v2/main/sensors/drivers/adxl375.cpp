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

// unused i think, should delete later
#define ADXL375_WRITE_ADDR (0xA6)
#define ADXL375_READ_ADDR (0xA7)

#define ADXL375_WHO_AM_I_REG (0x00)
#define ADXL375_WHO_AM_I_VAL (0xE5)
#define ADXL375_MG2G_MULTIPLIER (0.049) // 49mg per lsb

ADXL375::ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                 uint16_t adxl375_address, uint32_t scl_clk_speed) : config_{0, 0, false, 0x0A, 0x08, 0, 0x0B, 0}
{
    this->adxl375_dev_handle_ = i2c_create_device(port, addr_len, adxl375_address, scl_clk_speed);
}

ADXL375::ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                 uint16_t adxl375_address, uint32_t scl_clk_speed,
                 const ADXL375Config &cfg) : config_(cfg)
{
    this->adxl375_dev_handle_ = i2c_create_device(port, addr_len, adxl375_address, scl_clk_speed);
}

ADXL375::~ADXL375()
{
    i2c_remove_device(adxl375_dev_handle_);
}

uint8_t ADXL375::getDevID()
{
    uint8_t tmp[1] = {0};
    i2c_read(adxl375_dev_handle_, ADXL375_WHO_AM_I_REG, tmp, sizeof(tmp));
    return tmp[0];
}

sensor_type ADXL375::getType() const
{
    return ACCELEROMETER;
}

void ADXL375::configure()
{
    // not elegant but things get compolicated if you make i2c_write infer what the module wants
    uint8_t reg_and_data[] = {ADXL375_ACTIVITY_INACTIVITY_CTL, config_.activity_inactivity_cntl};
    i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));

    reg_and_data[0] = ADXL375_SHOCK_DETECTION_AXES_ENABLE;
    reg_and_data[1] = config_.enable_shock_detection;
    i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));

    if (config_.enable_low_power)
    {
        uint8_t res = BIT4 | config_.bw_output_rate;
        reg_and_data[0] = ADXL375_BW_RATE;
        reg_and_data[1] = res;
        i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));
    }
    else
    {
        reg_and_data[0] = ADXL375_BW_RATE;
        reg_and_data[1] = config_.bw_output_rate;
        i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));
    }

    reg_and_data[0] = ADXL375_POWER_CTL;
    reg_and_data[1] = config_.power_cntl;
    i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));

    reg_and_data[0] = ADXL375_ENABLE_INTERRUPTS;
    reg_and_data[1] = config_.enable_interrupts;
    i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));

    reg_and_data[0] = ADXL375_DATA_FORMAT;
    reg_and_data[1] = config_.data_format;
    i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));

    reg_and_data[0] = ADXL375_FIFO_CTL;
    reg_and_data[1] = config_.fifo_cntl;
    i2c_write(adxl375_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

sensor_status ADXL375::initialize()
{
    if (getDevID() != ADXL375_WHO_AM_I_VAL)
        return SENSOR_ERR_INIT;

    configure(); // applies default config if non provided on init

    return SENSOR_OK;
}

static void ADXL375::vreadTask(void *pvParameters)
{
    while (true)
    {
        sensor_reading curr_reading = read();
        ts_sensor_data *data = static_cast<ts_sensor_data *>(pvParameters);

        if (curr_reading.value.type != ACCELEROMETER)
            continue;

        if (curr_reading.status == SENSOR_OK)
        {
            data->hg_accel_x.store(curr_reading.value.data.accelerometerHG.accel[0], std::memory_order_relaxed);
            data->hg_accel_y.store(curr_reading.value.data.accelerometerHG.accel[1], std::memory_order_relaxed);
            data->hg_accel_z.store(curr_reading.value.data.accelerometerHG.accel[2], std::memory_order_relaxed);
        }
    }
}

sensor_reading ADXL375::read()
{
    sensor_reading result;
    result.value.type = ACCELEROMETER;

    uint8_t data_rd[6] = {0};
    i2c_read(adxl375_dev_handle_, ADXL375_ACCEL_X, data_rd, sizeof(data_rd));

    int16_t accel_x = ((int16_t)((data_rd[1] << 8) + (data_rd[0]))) * ADXL375_MG2G_MULTIPLIER;
    int16_t accel_y = ((int16_t)((data_rd[3] << 8) + (data_rd[2]))) * ADXL375_MG2G_MULTIPLIER;
    int16_t accel_z = ((int16_t)((data_rd[5] << 8) + (data_rd[4]))) * ADXL375_MG2G_MULTIPLIER;

    result.value.data.accelerometerHG.accel[0] = accel_x;
    result.value.data.accelerometerHG.accel[1] = accel_y;
    result.value.data.accelerometerHG.accel[2] = accel_z;

    result.status = SENSOR_OK; // this is a placeholder before we add error checking to the reads

    return result;
}