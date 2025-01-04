


void setup()
{
  //Open serial port (USB and wait for connection)
	Serial.begin(57600);
	while (!Serial); //Wait for serial port connection
  
  Serial.write("[apple,banana,brocoli]");
  Serial.write("[pineapple,turnip,swiss chard]");
  Serial.write("[carrot,mango,dragonfruit]");
  
}

void loop()
{

}
//Send Data to the pi
//data: the data to be written.  "," separates fields, "]" separates entries
//Will eventually take an array
/*void writeToPi(String data)
{
  Serial.write(data);
}*/