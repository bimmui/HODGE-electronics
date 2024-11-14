#include <stdio.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

class Sensor
{
public:
    Sensor(gpio_num_t pin) : pin_(pin)
    {
        esp_rom_gpio_pad_select_gpio(pin_);
        gpio_set_direction(pin_, GPIO_MODE_INPUT);
    }

    int read() const
    {
        return gpio_get_level(pin_);
    }

private:
    gpio_num_t pin_;
};

extern "C" void app_main()
{
    Sensor mySensor(GPIO_NUM_5); // Replace with your sensor's GPIO pin

    while (true)
    {
        int sensor_value = mySensor.read();
        std::cout << "Sensor Value: " << sensor_value << std::endl;
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay of 1 second
    }
}