/*
  RadioLib APRS Position Example

  This example sends APRS position reports 
  using SX1278's FSK modem. The data is
  modulated as AFSK at 1200 baud using Bell 
  202 tones.

  DO NOT transmit in APRS bands unless
  you have a ham radio license!

  Other modules that can be used for APRS:
  - SX127x/RFM9x
  - RF69
  - SX1231
  - CC1101
  - nRF24
  - Si443x/RFM2x
  - SX126x/LLCC68

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1278 radio = new Module(10, 2, 9, 3);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// create AFSK client instance using the FSK module
// this requires connection to the module direct
// input pin, here connected to Arduino pin 5
// SX127x/RFM9x:  DIO2
// RF69:          DIO2
// SX1231:        DIO2
// CC1101:        GDO2
// Si443x/RFM2x:  GPIO
// SX126x/LLCC68: DIO2
AFSKClient audio(&radio, 5);

// create AX.25 client instance using the AFSK instance
AX25Client ax25(&audio);

// create APRS client instance using the AX.25 client
APRSClient aprs(&ax25);

void setup() {
  Serial.begin(9600);

  // initialize SX1278
  // NOTE: moved to ISM band on purpose
  //       DO NOT transmit in APRS bands without ham radio license!
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.beginFSK(434.0);

  // when using one of the non-LoRa modules for AX.25
  // (RF69, CC1101, Si4432 etc.), use the basic begin() method
  // int state = radio.begin();

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // initialize AX.25 client
  Serial.print(F("[AX.25] Initializing ... "));
  // source station callsign:     "N7LEM"
  // source station SSID:         0
  // preamble length:             8 bytes
  state = ax25.begin("N7LEM");
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // initialize APRS client
  Serial.print(F("[APRS] Initializing ... "));
  // symbol:                      '>' (car)
  state = aprs.begin('>');
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

void loop() {
  Serial.println(F("[APRS] Sending location reports"));
  
  // send a location without message or timestamp
  char destination[] = "N0CALL";
  char latitude[] = "4911.67N";
  char longitude[] = "01635.96E";
  int state = aprs.sendPosition(destination, 0, latitude, longitude);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[APRS] Failed to send location, code "));
    Serial.println(state);
  }
  delay(500);
  
  // send a location with message and without timestamp
  char message[] = "I'm here!";
  state = aprs.sendPosition(destination, 0, latitude, longitude, message);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[APRS] Failed to send location and message code "));
    Serial.println(state);
  }
  delay(500);

  // you can also set repeater callsigns and SSIDs
  // up to 8 repeaters may be used
  // sendPosition will be sent with "WIDE2-2" path
  char* repeaterCallsigns[] = { "WIDE2" };
  uint8_t repeaterSSIDs[] = { 2 };
  aprs.useRepeaters(repeaterCallsigns, repeaterSSIDs, 1);
  
  // send a location with message and timestamp
  char timestamp[] = "093045z";
  state = aprs.sendPosition(destination, 0, latitude, longitude, message, timestamp);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[APRS] Failed to send location, message and timestamp code "));
    Serial.println(state);
  }
  delay(500);

  // when repeaters are no longer needed, they can be dropped
  aprs.dropRepeaters();

  // wait one minute before transmitting again
  Serial.println(F("[APRS] All done!"));
  delay(60000);
}

