// Example 1 - Receiving single characters

char receivedChar;
boolean newData = false;

void setup() {
    Serial.begin(9600);
}

void loop() {
    /*recvOneChar();
    showNewData();*/
      if (Serial.available() > 0) {
        Serial.println(Serial.read());
    }
      //Serial.println(Serial.read());
    
    
}

void recvOneChar() {
    if (Serial.available() > 0) {
        receivedChar = Serial.read();
        newData = true;
    }
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChar);
        newData = false;
    }
}

