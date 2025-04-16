#ifndef BMP581_H_
#define BMP581_H_

#include "peripherals/i2c_ex.h"
#include "./sensor_interface.h"

class BMP581 : public ApoSensor
{
public:
    BMP581(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
           uint16_t bmp581_address, uint32_t scl_clk_speed);
    ~BMP581();

    sensor_status initialize() override;
    sensor_reading read() override;
    static void vreadTask(void *pvParameters) override;
    sensor_type getType() const override;
    uint8_t getDevID() override;

private:
    float baro_offset_;
    i2c_master_dev_handle_t bmp581_dev_handle_;

    void configure() override;

    void calcBaroOffset();
    sensor_status softReset();
    sensor_status powerUpCheck();
    sensor_status checkHealth();
};
#endif