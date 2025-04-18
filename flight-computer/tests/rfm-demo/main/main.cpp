/*
   RadioLib Non-Arduino ESP-IDF Example

   This example shows how to use RadioLib without Arduino.
   In this case, a Liligo T-BEAM (ESP32 and SX1276)
   is used.

   Can be used as a starting point to port RadioLib to any platform!
   See this API reference page for details on the RadioLib hardware abstraction
   https://jgromes.github.io/RadioLib/class_hal.html

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "EspHal.h"
#include "sdkconfig.h"

// create a new instance of the HAL class
EspHal *hal = new EspHal(CONFIG_SPI_CLK, CONFIG_SPI_MISO, CONFIG_SPI_MOSI);

// now we can create the radio module
// NSS pin:   18
// DIO0 pin:  26
// NRST pin:  14
// DIO1 pin:  33
RFM96 radio = new Module(hal, CONFIG_RFM96_CHIP_SELECT, 5, CONFIG_RFM69_HARDWARE_RESET, RADIOLIB_NC);

static const char *TAG = "main";

// the entry point for the program
// it must be declared as "extern C" because the compiler assumes this will be a C function
extern "C" void app_main(void)
{
  // initialize just like with Arduino
  ESP_LOGI(TAG, "[RFM96] Initializing ... ");
  int state = radio.begin(434.550);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGI(TAG, "failed, code %d\n", state);
    while (true)
    {
      hal->delay(1000);
    }
  }
  ESP_LOGI(TAG, "success!\n");

  radio.setSpreadingFactor(7);
  radio.setBandwidth(125.0);
  radio.setCodingRate(7);

  // loop forever
  for (;;)
  {
    // send a packet
    ESP_LOGI(TAG, "[RFM96] Transmitting packet ... ");
    state = radio.transmit("Hello World!");
    if (state == RADIOLIB_ERR_NONE)
    {
      // the packet was successfully transmitted
      ESP_LOGI(TAG, "success!");
    }
    else
    {
      ESP_LOGI(TAG, "failed, code %d\n", state);
    }

    // wait for a second before transmitting again
    hal->delay(1000);
  }
}
