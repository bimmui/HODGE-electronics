// Example 1 - Receiving single characters

char receivedChar;
boolean newData = false;

void setup() {
    Serial.begin(9600);
    Serial.println("<Arduino is ready>");
}

void loop() {
    /*recvOneChar();
    showNewData();*/
    if(Serial.available() > 0)
    {
      Serial.read();
      Serial.println("Received!");
    }
    
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

