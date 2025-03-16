
int time = 0;
int velocity = 20;
int acceleration = 0;

void setup()
{
  //Open serial port (USB and wait for connection)
	Serial.begin(88600);
	while (!Serial); //Wait for serial port connection
  
}

void loop()
{
  String writeCommand = "";
  writeCommand = writeCommand + "" + time + "," + velocity + "," + acceleration + "\n";
  Serial.write(writeCommand.c_str());
  time++;
  velocity--;
  acceleration++;
  delay(10);
}
//Send Data to the pi
//data: the data to be written.  "," separates fields, "]" separates entries
//Will eventually take an array
/*void writeToPi(String data)
{
  Serial.write(data);
}*/