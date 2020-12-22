#include <avr/wdt.h> 
#include <VBUSDecoder.h>
VBUSDecoder vb;
#include <Arduino.h>
#include <TasmotaSlave.h>
#if !(defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__))
TasmotaSlave slave(&Serial);//Serial for Arduino UNO
#else
TasmotaSlave slave(&Serial3);//Serial3 para Arduino Mega
#endif
long lastTimevbus = 0;
long intervalvbus = 60000;
#define DEBUG 0

void setup() {
  
#if DEBUG
  Serial.begin(9600); 
  Serial.println("Arduino debugging started");
#endif  

#if !(defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__))
  Serial.begin(115200);  // Configure the Serial port for the correct baud rate. Serial for Arduino UNO
#else
  Serial3.begin(115200);  // Configure the Serial port for the correct baud rate. Serial3 for Arduino Mega
#endif

  vb.initialise();
  wdt_disable(); 
  wdt_enable(WDTO_8S); 

}

void loop() {
  slave.loop(); // Call the slave loop function every so often to process incoming requests
  vb.readSensor(); // Read the data from Vbus-UART in Serial1
  wdt_reset(); 
  if (millis() - lastTimevbus > intervalvbus) {
    lastTimevbus = millis();
    int S1Temp = vb.getS1Temp();
    int S2Temp = vb.getS2Temp();
    int S3Temp = vb.getS3Temp();
    int S4Temp = vb.getS4Temp();
    int P1Speed = vb.getP1Speed();
    char SystemTime[5]; // The buffer to copy the characters into
    vb.getSystemTime().toCharArray(SystemTime, 6); // To convert this string to a character array such that sprintf is expecting
    int P1 = vb.getP1OperatingHours();
    int P2 = vb.getP2OperatingHours();
    uint32_t HeatQuantity = vb.getHeatQuantity();
    char buffer[70];//sprintf works only on char array's
    sprintf(buffer, "{%02d,%02d,%02d,%02d,%s,%02d,%02d,%lu,%02d}", S1Temp, S2Temp, S3Temp, S4Temp, SystemTime, P1, P2, HeatQuantity, P1Speed); // Format data into character array
    slave.SendTele(buffer); // SendTele(char *data) publish character array via MQTT to Tasmota

#if DEBUG
    Serial.println("");
    Serial.println("*** Fresh Read ***");
    Serial.print("Collector Temp: ");
    Serial.println(vb.getS1Temp());
    Serial.print("Top Tank Temp: ");
    Serial.println(vb.getS3Temp());
    Serial.print("Bottom Tank Temp: ");
    Serial.println(vb.getS2Temp());
    Serial.print("Flow in Temp: ");
    Serial.println(vb.getS4Temp());
    Serial.print("Water Pump Status: ");
    Serial.println(vb.getP1Status());
    Serial.print("Water Pump Speed: ");
    Serial.println(vb.getP1Speed());
    Serial.print("Pump Hours Operation: ");
    Serial.println(vb.getP1OperatingHours());
    Serial.print("Backup Heater Hours Operation: ");
    Serial.println(vb.getP2OperatingHours());
    Serial.print("System Alert Status: ");
    Serial.println(vb.getAlertStatus());
    Serial.print("Scheme: ");
    Serial.println(vb.getScheme());
    Serial.print("System Time: ");
    Serial.println(vb.getSystemTime());
    Serial.print("HeatQuantity: ");
    Serial.println(vb.getHeatQuantity());
    Serial.println("*** End Read ***");
#endif
  }
}
