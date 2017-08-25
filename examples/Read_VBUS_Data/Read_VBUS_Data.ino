#include "VBUSDecoder.h"

VBUSDecoder vb;

void setup()
{

  Serial.begin(9600);
  vb.initialise();
}

void loop()
{
  vb.readSensor();

  Serial.println("");
  Serial.println("*** Fresh Read ***");
  Serial.print("Collector Temp: ");
  Serial.println(vb.getS1Temp());
  Serial.print("Top Tank Temp: ");
  Serial.println(vb.getS3Temp());
  Serial.print("Bottom Tank Temp: ");
  Serial.println(vb.getS2Temp());
  Serial.print("Water Pump Status: ");
  Serial.println(vb.getP1Status());
  Serial.print("Water Pump Speed: ");
  Serial.println(vb.getP1Speed());
  Serial.print("Pump Hours Operation: ");
  Serial.println(vb.getP1OperatingHours());
  Serial.print("System Alert Status: ");
  Serial.println(vb.getAlertStatus());
  Serial.print("System Time: ");
  Serial.println(vb.getSystemTime());
  Serial.println("*** End Read ***");

  delay(20000);
}
