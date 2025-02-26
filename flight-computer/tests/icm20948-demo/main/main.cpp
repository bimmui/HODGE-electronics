#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "icm20948.h"

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

void icm_read_task(void *args)
{
    ICM20948 *icm = static_cast<ICM20948 *>(args);

    icm20948_accel_value_t accels;
    icm20948_gyro_value_t gyros;

    for (int i = 0; i < 100; ++i)
    {
        icm->getAccel(&accels);
        ESP_LOGI(TAG, "ax: %lf ay: %lf az: %lf", accels.accel_x, accels.accel_y, accels.accel_z);
        icm->getGyro(&gyros);
        ESP_LOGI(TAG, "gx: %lf gy: %lf gz: %lf", gyros.gyro_x, gyros.gyro_y, gyros.gyro_z);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Ending task");
    vTaskDelete(NULL);
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting ICM test");
    i2c_bus_init();
    ESP_LOGI(TAG, "I2C bus initialization: good hopefully");
    static ICM20948 icm(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x68, CONFIG_I2C_MASTER_FREQUENCY);
    ESP_LOGI(TAG, "ICM20948 object initialized");
    icm.configureICM20948(ICM20948::ACCEL_FS_2G, ICM20948::GYRO_FS_1000DPS);

    xTaskCreate(icm_read_task, "icm read task", 1024 * 10, &icm, 15, NULL);
}