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
  // int state = radio.begin(434.550);
  float freq = 434.550;       // in MHz
  float bit_rate = 38.3606;   // in kbps
  float freq_dev = 20.507812; // in kHz
  float rx_bw = 100.0;        // in kHz
  int power = 20;             // in dBm
  int preamble_len = 4;
  int shaping = 0; // 0 = no shaping

  // initialize radio in FSK mode
  //   dataShaping is a float typically 0.0, 0.3, 0.5, 1.0, etc. for Gaussian filters
  // TODO: figure out the value to put for Gaussian filters that the cots gps tracker uses
  int state = radio.beginFSK(freq, bit_rate, freq_dev, rx_bw, power, preamble_len, 0.0);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGI(TAG, "failed, code %d\n", state);
    while (true)
    {
      hal->delay(1000);
    }
  }
  ESP_LOGI(TAG, "success!\n");

  // lora specific settings
  // radio.setSpreadingFactor(7);
  // radio.setBandwidth(125.0);
  // radio.setCodingRate(7);

  // fsk specific settings
  radio.setPreambleLength(16);
  // For sync word: the CC115L might use 2 or 3 bytes. Example:
  radio.setSyncWord(0x2D, 0xD4);
  radio.setDataShaping(0.5);                // if TX uses GFSK shaping
  radio.setEncoding(RADIOLIB_ENCODING_NRZ); // or whatever the TX uses
  radio.setCrcFiltering(false);

  // loop forever
  for (;;)
  {
    uint8_t buffer[8];
    int state = radio.receive(buffer, 8);

    // literally stripped this from the rfm69 receive example from radiolib example and modded print statements
    // TODO: FUCKING TEST THIS SHIT FUCK
    if (state == RADIOLIB_ERR_NONE)
    {
      // packet was successfully received
      ESP_LOGI(TAG, "success!");

      // print the data of the packet
      ESP_LOGD(TAG, "[RFM96] Data:\t\t%.*s", sizeof(buffer), (char *)buffer);

      // print RSSI (Received Signal Strength Indicator)
      // of the last received packet
      ESP_LOGD(TAG, "[RFM96] RSSI:\t\t%f dBm", radio.getRSSI());
    }
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
      // timeout occurred while waiting for a packet
      ESP_LOGW(TAG, "timeout!");
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
      // packet was received, but is malformed
      ESP_LOGW(TAG, "CRC error!");
    }
    else
    {
      // some other error occurred
      ESP_LOGW(TAG, "failed, code %d", state);
    }

    // wait for a second before transmitting again
    hal->delay(1000);
  }
}
