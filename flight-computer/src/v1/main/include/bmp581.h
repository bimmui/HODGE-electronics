#ifndef BMP581_H_
#define BMP581_H_

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <sdkconfig.h>
#include <sys/time.h>

typedef struct
{
    float pressure;
    float temperature_c;
    float temperature_f;
    float altitude;
    uint32_t raw_pressure;
} bmp581_data;

class BMP581
{
public:
    BMP581(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
           uint16_t bmp581_address, uint32_t scl_clk_speed);
    ~BMP581();
    int soft_reset(void);
    void bmp581_configure(void);
    bmp581_data bmp581_get_sample(void);
    uint32_t whoami(void);
    int power_up_check(void);

private:
    i2c_addr_bit_len_t bmp581_addr_len;
    uint16_t bmp581_addr;
    i2c_master_dev_handle_t bmp581_dev_handle;
};
#endif /* BMP581_H_ */