#ifndef vbusdecoder_h
#define vbusdecoder_h

#include <Arduino.h>


#if !(defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__))
#include <AltSoftSerial.h>
#endif

// Settings for the VBus decoding
#define FLength 6				// Framelength
#define FOffset 10				// Offset start of Frames
#define FSeptet 4				// Septet byte in Frame
#define SENSORNOTCONNECTED 8888 // Sometimes this might be 888 instead.

class VBUSDecoder
{
  public:
	bool initialise();
	float getS1Temp();
	float getS2Temp();
	float getS3Temp();
	float getS4Temp();
	bool readSensor();
	bool getP1Status();
	bool getP2Status();
	bool getAlertStatus();
	int getP1Speed();
	int getP2Speed();
	int getP1OperatingHours();
	int getP2OperatingHours();
	String getSystemTime();

  protected:
  private:

	#if !(defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__))
	AltSoftSerial Serial1;
	#endif

	bool relayPump = false;
	bool relay3WayValve = false;
	bool SystemAlert = false;

	float Sensor1_temp;
	float Sensor2_temp;
	float Sensor3_temp;
	float Sensor4_temp;

	float Sensor1_temp_max;
	float Sensor2_temp_max;
	float Sensor3_temp_max;
	float Sensor4_temp_max;

	// Conergy DT5 specific
	char PumpSpeed1; // in  %
	char PumpSpeed2; //  in %
	char RelaisMask;
	char ErrorMask;
	char Scheme;
	char OptionPostPulse;
	char OptionThermostat;
	char OptionHQM;
	uint16_t OperatingHoursRelais1;
	uint16_t OperatingHoursRelais2;
	uint32_t HeatQuantity;
	uint16_t Version;
	uint16_t OperatingHoursRelais1Today;
	uint16_t SystemTime;

	char Relay1;	  // in  %
	char Relay2;	  //  in %
	char MixerOpen;   // in  %
	char MixerClosed; // in  %
	char SystemNotification;
	//

	// These are set neither to 'on' or 'off' at initialization so at startup the value
	// from the first valid datagram will be sent to Domoticz.
	String lastRelay1 = "1";
	String lastRelay2 = "1";
	String lastSystemAlert = "1";

	unsigned char Buffer[80];
	volatile unsigned char Bufferlength;

	unsigned int Destination_address;
	unsigned int Source_address;
	unsigned char ProtocolVersion;
	unsigned int Command;
	unsigned char Framecnt;
	unsigned char Septet;
	unsigned char Checksum;
	long lastTimeTimer;
	long timerInterval;

	void clearMaxValues();
	bool vBusRead();
	float calcTemp(int Byte1, int Byte2);
	void InjectSeptet(unsigned char *Buffer, int Offset, int Length);
	void PrintHex8(unsigned char *data, uint8_t length); // prints 8-bit data in hex with leading zeroes
};

#endif
