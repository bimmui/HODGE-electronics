#include "tmp1075.h"

#define TMP1075_WHO_AM_I_VAL 0xE5

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
#define TMP1075_CONFIG_MASK ((uint16_t)0x6800)

TMP1075::TMP1075(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                 uint16_t adxl375_address, uint32_t scl_clk_speed)
{
    this->tmp1075_dev_handle_ = i2c_create_device(port, addr_len, adxl375_address, scl_clk_speed);
}

TMP1075::~TMP1075()
{
    i2c_remove_device(tmp1075_dev_handle_);
}

uint8_t TMP1075::getDevID()
{
    uint8_t tmp[1] = {0};
    i2c_read(tmp1075_dev_handle_, TMP1075_DEVID_REG, tmp, sizeof(tmp));
    return tmp[0];
}

sensor_type TMP1075::getType() const
{
    return TEMPERATURE;
}

void TMP1075::configure()
{
    // not gonna bother setting up the high limit nor the
    // low limit registers bc the alert pin isnt even connected

    // not using activity nor inactivity control EITHER
    uint8_t lsb = TMP1075_CONFIG_MASK & 0xFF;
    uint8_t msb = TMP1075_CONFIG_MASK >> 8;

    const uint8_t reg_and_data[] = {TMP1075_CONFIG_REG, lsb, msb};
    i2c_write(tmp1075_dev_handle_, reg_and_data, sizeof(reg_and_data));
}

sensor_status TMP1075::initialize()
{
    if (getDevID() != TMP1075_WHO_AM_I_VAL)
        return SENSOR_ERR_INIT;

    configure();

    return SENSOR_OK;
}

static void TMP1075::vreadTask(void *pvParameters)
{
    while (true)
    {
        sensor_reading curr_reading = read();
        ts_sensor_data *data = static_cast<ts_sensor_data *>(pvParameters);

        if (curr_reading.value.type != TEMPERATURE)
            continue;

        if (curr_reading.status == SENSOR_OK)
        {
            data->temp_temp_c.store(curr_reading.value.data.temp.temp_c, std::memory_order_relaxed);
        }
    }
}

sensor_reading TMP1075::read()
{
    sensor_reading result;
    result.value.type = TEMPERATURE;

    uint8_t tmp[2] = {0};
    esp_err_t success = i2c_read(tmp1075_dev_handle_, TMP1075_TEMP_REG, tmp, sizeof(tmp));

    if (success != ESP_OK)
    {
        result.status = SENSOR_ERR_READ;
        return result;
    }

    // first 4 lsb arent used
    int16_t ntmp = (int16_t)((tmp[1] << 4) + (tmp[0]));
    float thing = (ntmp * 0.0625f); // TODO: ts still dont work

    result.value.data.temp.temp_c = thing;

    result.status = SENSOR_OK; // this is a placeholder before we add error checking to the reads

    return result;
}