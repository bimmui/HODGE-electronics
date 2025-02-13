/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <math.h>
#include "bmp581.h"

/* Initalization defines */
#define I2C_MASTER_PORT                 I2C_NUM_0
#define I2C_MASTER_SCL_IO               22
#define I2C_MASTER_SDA_IO               21
#define MASTER_TRANSMIT_TIMEOUT         (500)


/* Function contracts */
static void i2c_controller_init(void);

static void i2c_controller_init(void)
{
    /* I was able to get the correct ordering from 
    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html#_CPPv4N23i2c_master_bus_config_t8i2c_portE*/
    i2c_master_bus_config_t i2c_controller_config = {};
    i2c_controller_config.i2c_port = I2C_MASTER_PORT;
    i2c_controller_config.sda_io_num = static_cast<gpio_num_t>(I2C_MASTER_SDA_IO);
    i2c_controller_config.scl_io_num = static_cast<gpio_num_t>(I2C_MASTER_SCL_IO);
    i2c_controller_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_controller_config.glitch_ignore_cnt = 7;
    i2c_controller_config.flags.enable_internal_pullup = true;

    /* Create new I2C bus in master mode */
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_controller_config, &bus_handle));
}

extern "C" void app_main(void)
{
    
    /* Let's first reset and then wait */
    i2c_controller_init();

    /* We initalize the peripheral reset and then reinit */
    static BMP581 bmp581(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x46, 10000);

    /* We are ready to configure*/
    bmp581.bmp581_configure();

    /* Let's start grabbing samples */
    bmp581_data sample;

    for (int i = 0; i < 20; i++){
        vTaskDelay(pdMS_TO_TICKS(1000));
        sample = bmp581.bmp581_get_sample();
        // printf("%lu\n", sample.raw_pressure);
        printf("%" PRIu32 "\n",sample.raw_pressure);
        // printf("%ld\n", sample.pressure);
        // printf("%ld\n", sample.temperature);
        // printf("Estimated Altitude: %.2f meters\n", sample.altitude);
    }
}
