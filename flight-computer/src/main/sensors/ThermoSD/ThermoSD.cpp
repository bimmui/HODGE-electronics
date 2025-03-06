/* 
* Author(s): Riley Becker
* Date: 2/23/25
* 
* Purpose: Logs temperature data from a MAX31856 thermocouple to an SD card
* through an ESP32 Wroom-32 Microcontroller
*/

//Upload for the SD card module
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#define SCK  14
#define MISO  12
#define MOSI  13
#define CS  15
SPIClass spi = SPIClass(HSPI);

// Upload MAX31856 library to use
#include <Adafruit_MAX31856.h>

//CS for the SD card: 27
const int chipSelect = 15;

//CS for the thermocouple: 5
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(5);
//Functions
void appendFile(fs::FS &fs, const char * path, const char * message);
void testFileIO(fs::FS &fs, const char * path);
void writeFileIndicateError(fs::FS &fs, const char * path, const char * message);

/* 
 * setup()
 * Purpose: Opens the CSV file, checks if SD card inserted
 * Parameters: None
 * Returns: Nothing, void
 * Effects: None
*/
void setup() {
  delay(2000);
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  //Checks if SD card exists
  spi.begin(SCK, MISO, MOSI, CS);
  if (!SD.begin(CS,spi,80000000)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  // wait for Serial Monitor to connect. Needed for native USB port boards only:
  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
  //Checks the type of MAX31856
  switch (maxthermo.getThermocoupleType() ) {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }
  //Write the starting test to the csv
  writeFileIndicateError(SD, "/datalogTemp.csv", "Temperature (C)");
  appendFile(SD, "/datalogTemp.csv", "\n");
}

/* 
 * loop()
 * Purpose: Continuously loops and collects data while the program operates
 * Parameters: None
 * Returns: Nothing, void
 * Effects: None
 * 
*/
void loop() {
  // make a string for assembling the data to log:
  String dataString = String(maxthermo.readThermocoupleTemperature(), 2);

  //Print to serial port
  Serial.println(dataString);

  //Write into the file
  appendFile(SD, "/datalogTemp.csv", dataString.c_str());
  appendFile(SD, "/datalogTemp.csv", "\n");
}

/* 
 * writeFileIndicateError(fs::FS &fs, const char * path, const char * message)
 * Purpose: Makes a new CSV file and writes into it
 * Parameters: input filestream fs::FS, const char *path, const char * message
 * Returns: Nothing, void
 * Effects: Creates a new file into SD card
*/
void writeFileIndicateError(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();

}

/*
 * appendFile(fs::FS &fs, const char * path, const char * message)
 * Purpose: Adds new information into a file
 * Parameters: input filestream fs::FS, const char *path, const char * message
 * Returns: Nothing, void
 * Effects: Adds text into a file in SD Card
*/
void appendFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  file.print(message);
  file.close();
}