#include <cstdint>

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "esp_hal.h"
#include "sdkconfig.h"

class Radio {
public:
    Radio(EspHal* hal, uint32_t cs, uint32_t irq, uint32_t rst, uint32_t gpio) {
        radio = new Module(hal, cs, irq, rst, gpio);
        state = -1;
    }

    bool init(float freq = 433.0) {
        if (state != -1) return false;
        state = radio.begin(freq);
        return state == RADIOLIB_ERR_NONE;
    }

    bool set_spreading_factor(uint32_t spread = 7) {
        if (state == -1) return false;
        radio.setSpreadingFactor(spread);
        return true;
    }
private:
    RFM96 radio;
    int state;
}