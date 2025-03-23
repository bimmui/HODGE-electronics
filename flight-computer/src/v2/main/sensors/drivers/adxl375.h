#ifndef ADXL375_H
#define ADXL375_H

#include "./i2c_ex.h"
#include "sensor_interface.h"

struct ADXL375Config
{
    uint8_t activity_inactivity_cntl;
    uint8_t enable_shock_detection;
    bool enable_low_power;
    uint8_t bw_output_rate;
    uint8_t power_cntl;
    uint8_t enable_interrupts;
    uint8_t data_format;
    uint8_t fifo_cntl;
};

class ADXL375 : public ApoSensor
{
public:
    ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
            uint16_t adxl375_address, uint32_t scl_clk_speed);
    ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
            uint16_t adxl375_address, uint32_t scl_clk_speed,
            const ADXL375Config &cfg);
    ~ADXL375();

    sensor_type getType() const override;
    sensor_reading read() override;
    sensor_status initialize() override;
    uint8_t getDevID() override;

private:
    i2c_master_dev_handle_t adxl375_dev_handle_;

    ADXL375Config config_; // will contain default config on init
    void configure() override;
};

#endif