#ifndef ADXL375_H
#define ADXL375_H

#include "driver/gpio.h"
#include "driver/i2c_master.h"

// change address
#define ADXL375_I2C_ADDRESS 0x69
#define ADXL375_WHO_AM_I_VAL 0xE5
#define ADXL375_MG2G_MULTIPLIER (0.049f)

typedef struct
{
    float accel_x;
    float accel_y;
    float accel_z;
} adxl375_accel_value_t;

class ADXL375
{
public:
    ADXL375(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t adxl375_address, uint32_t scl_clk_speed);
    ~ADXL375();

    // prob no need for copy constructor and copy assignments, that'll make things messy
    // ICM20948(const ICM20948 &other);
    // ICM20948 &operator=(const ICM20948 &other);

    void configureADXL375();
    uint8_t getADXL375ID();
    void setDataFormat();
    void wakeup();

    void getAccel(adxl375_accel_value_t *accel_vals);

private:
    i2c_addr_bit_len_t adxl375_addr_len;
    uint16_t adxl375_addr;
    i2c_master_dev_handle_t adxl375_dev_handle;
};

#endif