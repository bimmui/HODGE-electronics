#ifndef TMP1075_H
#define TMP1075_H

#include "peripherals/i2c_ex.h"
#include "sensor_interface.h"

class TMP1075 : public ApoSensor
{
public:
    TMP1075(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t tmp1075_address, uint32_t scl_clk_speed);
    ~TMP1075();

    sensor_type getType() const override;
    sensor_reading read() override;
    sensor_status initialize() override;
    uint8_t getDevID() override;

private:
    i2c_master_dev_handle_t tmp1075_dev_handle_;

    void configure() override;
};

#endif