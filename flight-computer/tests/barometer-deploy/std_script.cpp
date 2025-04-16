#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "bmp581.h"

static const char *TAG = "example";

#define WINDOW_SIZE 500
#define I2C_MASTER_NUM I2C_NUM_0

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

extern "C" void app_main()
{
    i2c_bus_init();
    static BMP581 bmp581(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x46, CONFIG_I2C_MASTER_FREQUENCY);
    bmp581.bmp581_configure();
    bmp581_data sample;

    // cicular buffer btw
    float window[WINDOW_SIZE] = {0};
    int index = 0;
    int count = 0;

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(200));

        sample = bmp581.bmp581_get_sample();
        float value = sample.altitude;

        window[index] = value;
        index = (index + 1) % WINDOW_SIZE;
        if (count < WINDOW_SIZE)
            count++;

        // compute mean
        float sum = 0.0f;
        for (int i = 0; i < count; i++)
        {
            sum += window[i];
        }
        float mean = sum / count;

        // compute the sample standard deviation using Welford's method
        float variance_sum = 0.0f;
        for (int i = 0; i < count; i++)
        {
            float diff = window[i] - mean;
            variance_sum += diff * diff;
        }
        float std_dev = (count > 1) ? sqrt(variance_sum / (count - 1)) : 0.0f;

        printf("Reading: %.6f, Mean: %.6f, StdDev: %.6f\n", value, mean, std_dev);
        fflush(stdout);
    }
}
