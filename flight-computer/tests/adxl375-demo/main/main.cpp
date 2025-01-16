#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "adxl375.h"

#define I2C_MASTER_NUM I2C_NUM_0 // I2C port number for master dev

static const char *TAG = "icm test";

/**
 * @brief i2c master initialization
 */
void i2c_bus_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.i2c_port = I2C_MASTER_NUM;
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.scl_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SCL);
    i2c_mst_config.sda_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SDA);
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
}

void adxl_read_task(void *args)
{
    ADXL375 *adxl = static_cast<ADXL375 *>(args);

    adxl375_accel_value_t accels;

    for (int i = 0; i < 100; ++i)
    {
        adxl->getAccel(&accels);
        ESP_LOGI(TAG, "ax: %lf ay: %lf az: %lf", accels.accel_x, accels.accel_y, accels.accel_z);
    }

    ESP_LOGI(TAG, "Ending task");
    vTaskDelete(NULL);
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting ICM test");
    i2c_bus_init();
    ESP_LOGI(TAG, "I2C bus initialization: good hopefully");
    static ADXL375 adxl(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x69, CONFIG_I2C_MASTER_FREQUENCY);
    ESP_LOGI(TAG, "ADXL375 object initialized");
    adxl.configureADXL375();

    xTaskCreate(adxl_read_task, "adxl read task", 1024 * 10, &adxl, 15, NULL);
}