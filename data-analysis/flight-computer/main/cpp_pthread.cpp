/* pthread/std::thread example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <iostream>
#include <thread>
#include <memory>
#include <string>
#include <sstream>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#include "compress.h"
#include "lib.rs.h"

#define LED_PIN (gpio_num_t) 13

const auto sleep_time = std::chrono::seconds {
    return_five()
};

extern "C" void app_main(void)
{
    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    int ON = 0;

    // Let the main task do something too
    while (true) {
        ON = !ON;
        gpio_set_level(LED_PIN, ON);
        std::this_thread::sleep_for(sleep_time);
    }
}
