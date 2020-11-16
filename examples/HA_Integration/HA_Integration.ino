// Enable debug prints to serial monitor
#define MY_DEBUG // Comment out for production

#ifdef MY_DEBUG
#define DEBUG_PRINT_LN(x)  Serial.println(x)
#define DEBUG_PRINT(x)  Serial.print(x)
#define DEBUG_PRINT_HEX(x)  Serial.println(x, HEX)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINT_HEX(x)
#define DEBUG_PRINT_LN(x)
#endif


#include "VBUSDecoder.h"

VBUSDecoder vb;

const unsigned long SLEEP_TIME = 30000;

void setup() {
  Serial.begin(9600);
  vb.initialise();
  //Battery analogue setup
#if defined(__AVR_ATmega2560__)
  analogReference(INTERNAL1V1);
#else
  analogReference(INTERNAL);
#endif

}

void loop()
{
  vb.readSensor();

  Serial.print("\nPanel: " + String(vb.getS1Temp(), 2));
  Serial.print(" TankTop: " + String(vb.getS3Temp(), 2));
  Serial.print(" TankBottom: " + String(vb.getS2Temp(), 2));
  Serial.print(" PumpStatus: " + String(vb.getP1Status()));
  Serial.print(" PumpSpeed: " + String(vb.getP1Speed()));

  delay(SLEEP_TIME);
}
