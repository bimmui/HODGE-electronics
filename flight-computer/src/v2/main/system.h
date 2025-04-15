#pragma once

#include <stdio.h>
#include <memory>

#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "driver/i2c_master.h"

#include "sensors/drivers/adxl375.h"
#include "sensors/drivers/bmp581.h"
#include "sensors/drivers/gps.h"
#include "sensors/drivers/icm20948.h"
#include "sensors/drivers/tmp1075.h"

#include "apo_aggregator.h"
#include "sd_manager.h"

struct startup_vals
{
    int32_t access_count;
    float G_offset[3];
    float A_B[3];
    float A_Ainv[3][3];
    float M_B[3];
    float M_Ainv[3][3];
};

startup_vals read_nvs_startup_data(void)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_open(CONFIG_NVS_NAMESPACE, NVS_READONLY, &h));

    startup_vals fin;

    // read and print access counter
    nvs_get_i32(h, KEY_COUNT, &fin.access_count);
    printf("NVS accessed %ld time(s)\n", fin.access_count);

    size_t size = sizeof(fin.G_offset);
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(h, CONFIG_KEY_G_OFFSET, fin.G_offset, &size));
    printf("gyro offset: ");
    for (int i = 0; i < 3; i++)
        printf("%d ", fin.G_offset[i]);
    printf("\n");

    size = sizeof(fin.A_B);
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(h, CONFIG_KEY_A_B, fin.A_B, &size));
    printf("accelerometer bias: ");
    for (int i = 0; i < 3; i++)
        printf("%d ", fin.A_B[i]);
    printf("\n");

    size = sizeof(fin.M_B);
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(h, CONFIG_KEY_M_B, fin.M_B, &size));
    printf("magnetometer bias: ");
    for (int i = 0; i < 3; i++)
        printf("%d ", fin.M_B[i]);
    printf("\n");

    size = sizeof(fin.A_Ainv);
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(h, CONFIG_KEY_A_AInv, fin.A_Ainv, &size));
    printf("accelerometer correction matrix:\n");
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
            printf("%.5f ", fin.A_Ainv[i][j]);
        printf("\n");
    }

    size = sizeof(fin.M_Ainv);
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_blob(h, CONFIG_KEY_M_Ainv, fin.M_Ainv, &size));
    printf("magnetometer correction matrix:\n");
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
            printf("%.5f ", fin.M_Ainv[i][j]);
        printf("\n");
    }

    nvs_close(h);

    return fin;
}

void init_sensors(ApoAggregator apo, SdCardManager sd)
{
    uint8_t count = apo.getNumSensors();
    char *sensor_stats[count] = apo.initializeSensors();

    for (int i = 0; i < count; i++)
    {
        if (sd.writeFile(CONFIG_INIT_FILE, sensor_stats[i]) != ESP_OK)
        {
            ESP_LOGE((const char *)"apo_init", "Error writing file.");
        }
    }
}

void SYS_INIT(ApoAggregator apo, SdCardManager sd, RFM96 radio)
{
    esp_err_t ret = sd.mount();
    if (ret != ESP_OK)
    {
        ESP_LOGE((const char *)"apo_init", "Failed to mount the SD card");
        return;
    }

    startup_vals fin = read_nvs_startup_data();
    i2c_bus_init();

    // create all the sensor objects and throw them into the apoaggregator

    static ADXL375 adxl(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, CONFIG_ADXL375_ADDRESS, CONFIG_I2C_MASTER_FREQUENCY);
    static ICM20948 icm(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, CONFIG_ICM20948_ADDRESS, CONFIG_I2C_MASTER_FREQUENCY);
    static BMP581 bmp(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, CONFIG_BMP581_ADDRESS, CONFIG_I2C_MASTER_FREQUENCY);
    static TMP1075 temperature(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, CONFIG_TMP1075_ADDRESS, CONFIG_I2C_MASTER_FREQUENCY);

    GpsNmeaConfig cfg = GpsNmeaConfigDefault();
    static GpsSensor gps(cfg);

    icm.setCalibrationFactors(fin.G_offset, fin.A_B, fin.A_Ainv, fin.M_B, fin.M_B);

    apo.addSensor(&adxl);
    apo.addSensor(&icm);
    apo.addSensor(&bmp);
    apo.addSensor(&temperature);
    apo.addSensor(&gps);

    init_sensors(apo, sd);

    sd.writeFile(CONFIG_INIT_FILE, "[RFM96] Initializing ... ");
    // ESP_LOGI(TAG, "[RFM96] Initializing ... ");
    int state = radio.begin(434.550);
    char err_str[32];
    if (state != RADIOLIB_ERR_NONE)
    {
        snprintf(err_str, sizeof(err_str), "failed, code %d\n", state);
        sd.writeFile(CONFIG_INIT_FILE, err_str);
        // ESP_LOGI(TAG, "failed, code %d\n", state);
        while (true)
        {
            hal->delay(1000);
        }
    }
    else
    {
        sd.writeFile(CONFIG_INIT_FILE, "success!\n");
    }

    radio.setSpreadingFactor(7);
    radio.setBandwidth(125.0);
    radio.setCodingRate(7);
}