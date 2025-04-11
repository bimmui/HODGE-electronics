#pragma once

#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "apo_aggregator.h"
#include "sd_manager.h"
#include "hal.h"

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