#include "driver/gpio.h"
#include "gpio.h"

void set_pin(int32_t pin, int32_t level) {
    gpio_set_level((gpio_num_t) pin, level);
}