#ifndef TMP1075_H
#define TMP1075_H

#include "driver/gpio.h"
#include "driver/i2c_master.h"

// change address
#define TMP1075_I2C_ADDRESS 0x69
#define TMP1075_WHO_AM_I_VAL 0xE5

class TMP1075
{
public:
    TMP1075(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t tmp1075_address, uint32_t scl_clk_speed);
    ~TMP1075();

    // prob no need for copy constructor and copy assignments, that'll make things messy
    // ICM20948(const ICM20948 &other);
    // ICM20948 &operator=(const ICM20948 &other);

    void configureTMP1075();
    float readTempC();
    float readTempF();

    uint8_t getTMP1075ID();

private:
    i2c_addr_bit_len_t tmp1075_addr_len;
    uint16_t tmp1075_addr;
    i2c_master_dev_handle_t tmp1075_dev_handle;
};

#endif