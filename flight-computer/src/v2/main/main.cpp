#include <cstdio>
#include <iostream>
#include <string>

#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"

#include <RadioLib.h>

#include "systems.h"
#include "apo_aggregator.h"
#include "sd_manager.h"
#include "EspHal.h"

#define MILLIS() (esp_timer_get_time() / 1000ULL)

extern "C" void app_main()
{
    ApoAggregator apo;
    SdCardManager sd;
    EspHal *hal = EspHal(CONFIG_SPI_CLK, CONFIG_SPI_MISO, CONFIG_SPI_MOSI);
    RFM96 radio = Module(hal, CONFIG_RFM96_CHIP_SELECT, 5, CONFIG_RFM69_HARDWARE_RESET, RADIOLIB_NC);

    SYS_INIT(apo, sd, radio);
}