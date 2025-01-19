#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "tmp1075.h"

#define I2C_MASTER_NUM I2C_NUM_0 // I2C port number for master dev

static const char *TAG = "tmp test";

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

void tmp_read_task(void *args)
{
    TMP1075 *tmp = static_cast<TMP1075 *>(args);

    for (int i = 0; i < 100; ++i)
    {
        float val = tmp->readTempC();
        ESP_LOGI(TAG, "temp in C: %lf", val);
    }

    ESP_LOGI(TAG, "Ending task");
    vTaskDelete(NULL);
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting ICM test");
    i2c_bus_init();
    ESP_LOGI(TAG, "I2C bus initialization: good hopefully");
    static TMP1075 tmp(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x69, CONFIG_I2C_MASTER_FREQUENCY);
    ESP_LOGI(TAG, "TMP1075 object initialized");
    tmp.configureTMP1075();

    xTaskCreate(tmp_read_task, "tmp read task", 1024 * 10, &tmp, 15, NULL);
}