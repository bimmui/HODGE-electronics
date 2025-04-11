#pragma once

#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "apo_aggregator.h"
#include "sd_manager.h"
#include "hal.h"

#define NVS_NAMESPACE "storage"
#define KEY_COUNT "access_count"
#define KEY_G_OFFSET "gyro_offset"
#define KEY_A_B "accel_bias"
#define KEY_A_AInv "accel_inverted" // TODO: check out the name of this thing
#define KEY_M_B "mag_bias"
#define KEY_M_AInv "mag_inverted" // TODO: same as above

#define KEY_ARRAY1D "array_1d"
#define KEY_ARRAY2D "array_2d"
#define KEY_NUMBER "some_number"

struct
{
    int verbose;
    int max_retries;
} startup;

void read_nvs_data(void)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READONLY, &h));

    // read and print access counter
    int32_t access_count = 0;
    nvs_get_i32(h, KEY_COUNT, &access_count);
    printf("NVS accessed %ld time(s)\n", access_count);

    // 1D array
    int arr1d[5];
    size_t size = sizeof(arr1d);
    ESP_ERROR_CHECK(nvs_get_blob(h, KEY_ARRAY1D, arr1d, &size));
    printf("1D array: ");
    for (int i = 0; i < 5; i++)
        printf("%d ", arr1d[i]);
    printf("\n");

    // 2D array
    float arr2d[2][3];
    size = sizeof(arr2d);
    ESP_ERROR_CHECK(nvs_get_blob(h, KEY_ARRAY2D, arr2d, &size));
    printf("2D array:\n");
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 3; j++)
            printf("%.2f ", arr2d[i][j]);
        printf("\n");
    }

    // that single number
    double value;
    size = sizeof(value);
    ESP_ERROR_CHECK(nvs_get_blob(h, KEY_NUMBER, &value, &size));
    printf("Stored number = %.5f\n", value);

    nvs_close(h);
}

void SYS_INIT(ApoAggregator apo, SdCardManager sd)
{
    esp_err_t ret = sd.mount();
    if (ret != ESP_OK)
    {
        ESP_LOGE((const char *)"apo_init", "Failed to mount the SD card. Stopping app.");
        return;
    }

    uint8_t count = apo.getNumSensors();
    char *sensor_stats[count] = apo.initializeSensors();

    for (int i = 0; i < count; i++)
    {
        if (sd.writeFile(INIT_FILE, sensor_stats[i]) != ESP_OK)
        {
            ESP_LOGE((const char *)"apo_init", "Error writing file.");
        }
    }
}