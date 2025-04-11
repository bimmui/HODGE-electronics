#include <stdio.h>
#include <iostream>
#include <driver/i2c_master.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#define MILLIS() (esp_timer_get_time() / 1000ULL)

// sensor drivers
#include "apo_agreggator.h"
#include "systems.h"

extern "C" void app_main()
{
    ApoAggregator apo;
    SdCardManager sd;

    SYS_INIT(apo, sd);
}