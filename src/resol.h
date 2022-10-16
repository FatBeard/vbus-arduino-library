#include "esphome.h"

// Settings for the VBus decoding
#define FLength 6 // Framelength
#define FOffset 10 // Offset start of Frames
#define FSeptet 4 // Septet byte in Frame
#define SENSORNOTCONNECTED 8888 // Sometimes this might be 888 instead.

class Resol: public PollingComponent, public UARTDevice, public Sensor {

  public: Resol(UARTComponent * parent): PollingComponent(30000),
  UARTDevice(parent) {};

  Sensor * panel_temperature = new Sensor();
  Sensor * hot_water = new Sensor();
  Sensor * bottom_tank = new Sensor();
  Sensor * solar_status = new Sensor();

  float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

  void setup() override {

    initialise();
  }

  void update() override {

    readSensor();

    panel_temperature -> publish_state(getS1Temp());
    hot_water -> publish_state(getS3Temp());
    bottom_tank -> publish_state(getS2Temp());
    solar_status -> publish_state(getP1Status());
  }

  bool initialise() {

    clearMaxValues();
    return true;
  } // end void initialise()

  float getS1Temp() {

    return sensor1Temp;
  }

  float getS2Temp() {
    return sensor2Temp;
  }

  float getS3Temp() {
    return sensor3Temp;
  }

  float getS4Temp() {
    return sensor4Temp;
  }

  bool getP1Status() {
    return relayPump;
  }

  bool getP2Status() {
    return relay3WayValve;
  }

  int getP1Speed() {

    return String(Relay1, DEC).toInt();
  }

  int getP2Speed() {

    return String(Relay2, DEC).toInt();
  }
  int getP1OperatingHours() {

    return String(OperatingHoursRelay1, DEC).toInt();
  }

  int getP2OperatingHours() {

    return String(OperatingHoursRelay2, DEC).toInt();
  }

  int getScheme() {

    return String(Scheme, DEC).toInt();
  }

  bool getAlertStatus() {
    return SystemAlert;
  }

  String getSystemTime() {

    int hours = SystemTime / 60;
    int minutes = SystemTime % 60;
    String toReturn = String(String(hours) + ":" + String(minutes));

    if (hours < 10) {
      toReturn = "0" + toReturn;
    }

    return toReturn;
  }

  uint32_t getHeatQuantity() {
    return HeatQuantity;
  }

  bool readSensor() {

    if (vBusRead()) {
      #if DEBUG
      Serial.println("------Decoded VBus data------");
      Serial.print("Destination: ");
      Serial.println(Destination_address, HEX);
      Serial.print("Source: ");
      Serial.println(Source_address, HEX);
      Serial.print("Protocol Version: ");
      Serial.println(ProtocolVersion);
      Serial.print("Command: ");
      Serial.println(Command, HEX);
      Serial.print("Framecount: ");
      Serial.println(Framecnt);
      Serial.print("Checksum: ");
      Serial.println(Checksum);
      Serial.println("------Values------");
      Serial.print("Sensor 1: ");
      Serial.println(sensor1Temp);
      Serial.print("Sensor 2: ");
      Serial.println(sensor2Temp);
      Serial.print("Sensor 3: ");
      Serial.println(sensor3Temp);
      Serial.print("Sensor 4: ");
      Serial.println(sensor4Temp);
      Serial.print("Relay 1: ");
      Serial.println(Relay1, DEC);
      Serial.print("Relay 2: ");
      Serial.println(Relay2, DEC);
      Serial.print("Minute of Day: ");
      Serial.println(SystemTime);
      Serial.print("Notifications: ");
      Serial.println(SystemNotification, DEC);
      Serial.println("------END------");
      #endif

    } //end VBusRead

    /*
       S1 = Sensor 1 (sensor SFB/stove)
       S2 = Sensor 2 (sensor store base)
       S3 = Sensor 3 (sensor store top)
       S4 = Sensor 4 (system-dependent)
       R1 = Pump
       R2 = 3-way valve
    */

    // Convert relay value to On or Off.
    if (Relay1 != 0x00) {
      relayPump = true;
    } else {
      relayPump = false;
    }

    if (Relay2 == 0x64) {
      relay3WayValve = true;
    } else if (Relay2 == 0x00) {
      relay3WayValve = false;
    } else {
      relay3WayValve = false;
    }

    if (SystemNotification != 0x00 || ErrorMask != 0x00) // Not really sure what ErrorMask is, treating as system alert.
    {
      SystemAlert = true;
    } else {
      SystemAlert = false;
    }

    return true;
  }

  protected:

    private:

    bool relayPump = false;
  bool relay3WayValve = false;
  bool SystemAlert = false;

  float sensor1Temp;
  float sensor2Temp;
  float sensor3Temp;
  float sensor4Temp;

  float sensor1TempMax;
  float sensor2TempMax;
  float sensor3TempMax;
  float sensor4TempMax;

  // Conergy DT5 specific
  char PumpSpeed1; // in  %
  char PumpSpeed2; //  in %
  char RelaisMask;
  char ErrorMask;
  char Scheme;
  char OptionPostPulse;
  char OptionThermostat;
  char OptionHQM;
  uint16_t OperatingHoursRelay1;
  uint16_t OperatingHoursRelay2;
  uint32_t HeatQuantity;
  uint16_t Version;
  uint16_t OperatingHoursRelais1Today;
  uint16_t SystemTime;

  char Relay1; // in  %
  char Relay2; //  in %
  char MixerOpen; // in  %
  char MixerClosed; // in  %
  char SystemNotification;
  //

  // These are set neither to 'on' or 'off' at initialization so at startup the value
  // from the first valid datagram will be sent to Domoticz.
  String lastRelay1 = "1";
  String lastRelay2 = "1";
  String lastSystemAlert = "1";

  unsigned char Buffer[90];
  volatile unsigned char Bufferlength;

  unsigned int Destination_address;
  unsigned int Source_address;
  unsigned char ProtocolVersion;
  unsigned int Command;
  unsigned char Framecnt;
  unsigned char Septet;
  unsigned char Checksum;
  long lastTimeTimer;
  long timerInterval = 200;

  // Clear all maximum values
  void clearMaxValues() {
    sensor1TempMax = 0.0;
    sensor2TempMax = 0.0;
    sensor3TempMax = 0.0;
    sensor4TempMax = 0.0;
  }
  // The following function reads the data from the bus and converts it all
  // depending on the used VBus controller.
  bool vBusRead() {
    int F;
    char c;
    const char sync1 = 0xAA;

    bool start = true;
    bool stop = false;
    bool quit = false;
    int Bufferlength = 0;
    unsigned long lastTimeTimer = millis();

    while ((!stop) and(!quit)) {
      if (available()) {
        c = read();

        if (c == sync1) {

          #if DEBUG
          // Serial.println("Sync found");
          #endif

          if (start) {
            start = false;
            Bufferlength = 0;

          } else {
            if (Bufferlength < 20) {
              lastTimeTimer = millis();
              Bufferlength = 0;

            } else {
              stop = true;
              
              }
          }
        }
  
  
        if ((!start) and(!stop)) {
          Buffer[Bufferlength] = c;
          Bufferlength++;
        }
      }
      if ((timerInterval > 0) && (millis() - lastTimeTimer > timerInterval)) {

        ESP_LOGD("Deltasol","Timed out waiting for data.");

      }
    }

    lastTimeTimer = 0;

    if (!quit) {

      Destination_address = Buffer[2] << 8;
      Destination_address |= Buffer[1];
      Source_address = Buffer[4] << 8;
      Source_address |= Buffer[3];
      ProtocolVersion = (Buffer[5] >> 4) + (Buffer[5] & (1 << 15));

      Command = Buffer[7] << 8;
      Command |= Buffer[6];
      Framecnt = Buffer[8];
      Checksum = Buffer[9];
      #if DEBUG
      Serial.println("---------------");
      Serial.print("Destination: ");
      Serial.println(Destination_address, HEX);
      Serial.print("Source: ");
      Serial.println(Source_address, HEX);
      Serial.print("Protocol Version: ");
      Serial.println(ProtocolVersion);
      Serial.print("Command: ");
      Serial.println(Command, HEX);
      Serial.print("Framecount: ");
      Serial.println(Framecnt);
      Serial.print("Checksum: ");
      Serial.println(Checksum);
      Serial.println("---------------");

      #endif
      // Only analyse Commands 0x100 = Packet Contains data for slave
      // with correct length = 10 bytes for HEADER and 6 Bytes  for each frame

      if ((Command == 0x0100) and(Bufferlength == 10 + Framecnt * 6) and(Checksum == VBus_CalcCrc(Buffer, 1, 8))) {

        //Only decode the data from the correct source address
        //(There might be other VBus devices on the same bus).

        if (Source_address == 0x3271) {
          #if DEBUG
          Serial.println("---------------");
          Serial.println("Now decoding for 0x3271");
          Serial.println("---------------");

          #endif

          // Frame info for the Resol ConergyDT5
          // check VBusprotocol specification for other products

          // This library is made for the ConergyDT5 (0x3271)

          //Offset  Size    Mask    Name                    Factor  Unit
          //0       2               Temperature sensor 1    0.1     &#65533;C
          //2       2               Temperature sensor 2    0.1     &#65533;C
          //4       2               Temperature sensor 3    0.1     &#65533;C
          //6       2               Temperature sensor 4    0.1     &#65533;C
          //8       1               Pump speed pump         1       1
          //9       1               Pump speed pump 2       1
          //10      1               Relay mask              1
          //11      1               Error mask              1
          //12      2               System time             1
          //14      1               Scheme                  1
          //15      1       1       Option PostPulse        1
          //15      1       2       Option thermostat       1
          //15      1       4       Option HQM              1
          //16      2               Operating hours relay 1 1
          //18      2               Operating hours relay 2 1
          //20      2               Heat quantity           1       Wh
          //22      2               Heat quantity           1000    Wh
          //24      2               Heat quantity           1000000 Wh
          //26      2               Version 0.01
          //
          // Each frame has 6 bytes
          // byte 1 to 4 are data bytes -> MSB of each bytes
          // byte 5 is a septet and contains MSB of bytes 1 to 4
          // byte 6 is a checksum
          //
          //*******************  Frame 1  *******************
          F = FOffset;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            // 'collector1' Temperatur Sensor 1, 15 bits, factor 0.1 in C
            sensor1Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            // 'store1' Temperature sensor 2, 15 bits, factor 0.1 in C
            sensor2Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 2  *******************
          F = FOffset + FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor3Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor4Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 3  *******************
          F = FOffset + FLength * 2;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            PumpSpeed1 = (Buffer[F]);
            PumpSpeed2 = (Buffer[F + 1]);
            RelaisMask = Buffer[F + 2];
            ErrorMask = Buffer[F + 3];
          }
          //*******************  Frame 4  *******************
          F = FOffset + FLength * 3;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            SystemTime = Buffer[F + 1] << 8 | Buffer[F];
            Scheme = Buffer[F + 2];
            OptionPostPulse = (Buffer[F + 3] & 0x01);
            OptionThermostat = ((Buffer[F + 3] & 0x02) >> 1);
            OptionHQM = ((Buffer[F + 3] & 0x04) >> 2);
          }
          //*******************  Frame 5  *******************
          F = FOffset + FLength * 4;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            OperatingHoursRelay1 = Buffer[F + 1] << 8 | Buffer[F];
            OperatingHoursRelay2 = Buffer[F + 3] << 8 | Buffer[F + 2];
          }
          //*******************  Frame 6  *******************
          F = FOffset + FLength * 5;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            HeatQuantity = (Buffer[F + 1] << 8 | Buffer[F]) + (Buffer[F + 3] << 8 | Buffer[F + 2]) * 1000;
          }
          //*******************  Frame 7  *******************
          F = FOffset + FLength * 6;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            HeatQuantity = HeatQuantity + (Buffer[F + 1] << 8 | Buffer[F]) * 1000000;
            Version = Buffer[F + 3] << 8 | Buffer[F + 2];
          }
          ///******************* End of frames ****************
        } // end 0x3271 Conenergy DT5
        else if (Source_address == 0x5611) {
          #if DEBUG
          Serial.println("---------------");
          Serial.println("Now decoding for 0x5611");
          Serial.println("---------------");
          #endif
          // Frame info for the Resol Deltatherm FK and Oranier Aquacontrol III
          // check VBusprotocol specification for other products

          //

          //Offset  Size    Mask    Name                    Factor  Unit
          // Frame 1
          //0       2               Temperature sensor 1    0.1     &#65533;C
          //2       2               Temperature sensor 2    0.1     &#65533;C
          // Frame 2
          //4       2               Temperature sensor 3    0.1     &#65533;C
          //6       2               Temperature sensor 4    0.1     &#65533;C
          // Frame 3
          //8       1               Relay 1                 1       %
          //9       1               Relay 2                 1       %
          //10      1               Mixer open              1       %
          //11      1               Mixer closed            1       %
          // Frame 4
          //12      4               System date             1
          // Frame 5
          //16      2               System time             1
          //18      1               System notification     1
          //
          // Each frame has 6 bytes
          // byte 1 to 4 are data bytes -> MSB of each bytes
          // byte 5 is a septet and contains MSB of bytes 1 to 4
          // byte 6 is a checksum
          //
          //*******************  Frame 1  *******************

          F = FOffset;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor1Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor2Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 2  *******************
          F = FOffset + FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor3Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor4Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 3  *******************
          F = FOffset + FLength * 2;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            // Some of the values are 7 bit instead of 8.
            // Adding '& 0x7F' means you are only interested in the first 7 bits.
            // 0x7F = 0b1111111.
            // See: http://stackoverflow.com/questions/9552063/c-language-bitwise-trick
            Relay1 = (Buffer[F] & 0X7F);
            Relay2 = (Buffer[F + 1] & 0X7F);
            MixerOpen = (Buffer[F + 2] & 0X7F);
            MixerClosed = (Buffer[F + 3] & 0X7F);
          }
          //*******************  Frame 4  *******************
          F = FOffset + FLength * 3;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            // System date is not needed for Domoticz
          }
          //*******************  Frame 5  *******************
          F = FOffset + FLength * 4;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            // System time is not needed for Domoticz

            // Status codes System Notification according to Resol:
            //0: no error / warning
            //1: S1 defect
            //2: S2 defect
            //3: S3 defect
            //4: VFD defect
            //5: Flow rate?
            //6: ΔT too high
            //7: Low water level

            SystemNotification = Buffer[F + 2];
          }
          ///******************* End of frames ****************

        } //End 0x5611 Resol DeltaTherm FK
        else if (Source_address == 0x4212) {

          #if DEBUG
          Serial.println("---------------");
          Serial.println("Now decoding for DeltaSol C 0x4212");
          Serial.println("---------------");
          #endif
          //*******************  Frame 1  *******************
          F = FOffset;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor1Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor2Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 2  *******************
          F = FOffset + FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor3Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor4Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 3  *******************
          F = FOffset + FLength * 2;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            Relay1 = (Buffer[F]);
            Relay2 = (Buffer[F + 1]);
            ErrorMask = Buffer[F + 2];
            Scheme = Buffer[F + 3];
          }
          //*******************  Frame 4  *******************
          F = FOffset + FLength * 3;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            OperatingHoursRelay1 = Buffer[F + 1] << 8 | Buffer[F];
            OperatingHoursRelay2 = Buffer[F + 3] << 8 | Buffer[F + 2];
          }
          //*******************  Frame 5  *******************
          F = FOffset + FLength * 4;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            HeatQuantity = (Buffer[F + 1] << 8 | Buffer[F]) + (Buffer[F + 3] << 8 | Buffer[F + 2]) * 1000;
          }
          //*******************  Frame 6  *******************
          F = FOffset + FLength * 5;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            HeatQuantity = HeatQuantity + (Buffer[F + 1] << 8 | Buffer[F]) * 1000000;
            SystemTime = Buffer[F + 3] << 8 | Buffer[F + 2];
          }
          //*******************  Frame 7  *******************
          F = FOffset + FLength * 6;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
          }
          ///******************* End of frames ****************

        } // end 0x4212 DeltaSol C
        else if (Source_address == 0x2211 or Source_address == 0x4211) {

          // Frame info for the Resol DeltaSol CS Plus (Joule) or DeltaSol BS Plus
          // check VBusprotocol specification for other products
          // This library is made for the Resol DeltaSol CS Plus (0x2211) or BS Plus (0x4211)
          //Offset  Mask        Name                Factor      Unit
          //0                   Temperature S1      1.0         °C
          //1                   Temperature S1      256.0       °C
          //2                   Temperature S2      1.0         °C
          //3                   Temperature S2      256.0       °C
          //4                   Temperature S3      1.0         °C
          //5                   Temperature S3      256.0       °C
          //6                   Temperature S4      1.0         °C
          //7                   Temperature S4      256.0       °C
          //8                   Pump Speed R1       1           %
          //10                  Operating Hours R1  1           h
          //11                  Operating Hours R1  256         h
          //12                  Pump Speed R2       1           %
          //14                  Operating Hours R2  1           h
          //15                  Operating Hours R2  256         h
          //16                  UnitType            1
          //17                  System              1
          //20          1       Sensor 1 defekt     1
          //20          2       Sensor 2 defekt     1
          //20          4       Sensor 3 defekt     1
          //20          8       Sensor 4 defekt     1
          //20                  Error Mask          1
          //20                  Error Mask          256
          //22                  Time                1
          //23                  Time                256
          //24                  Statusmask          1
          //25                  Statusmask          256
          //26                  Statusmask          65536
          //27                  Statusmask          16777216
          //28                  Heat Quantity       1           Wh
          //29                  Heat Quantity       256         Wh
          //30                  Heat Quantity       65536       Wh
          //31                  Heat Quantity       16777216    Wh
          //32                  SW-Version          0.01
          //33                  SW-Version          2.56
          //
          // Each frame has 6 bytes, FLength
          // byte 1 to 4 are data bytes -> MSB of each bytes
          // byte 5 is a septet and contains MSB of bytes 1 to 4, FSeptet
          // byte 6 is a checksum
          // FOffset 10, Offset start of Frames
          //*******************  Frame 1  *******************
          #if DEBUG
          Serial.println("---------------");
          Serial.println("Now decoding for DeltaSol CS Plus 0x2211 or BS Plus 0x4211");
          Serial.println("---------------");
          #endif
          //*******************  Frame 1  *******************
          F = FOffset;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor1Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor2Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 2  *******************
          F = FOffset + FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor3Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor4Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 3  *******************
          F = FOffset + FLength * 2;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            Relay1 = (Buffer[F]);
            OperatingHoursRelay1 = Buffer[F + 2] + Buffer[F + 3] * 256; //Numero de horas rele 1
            ErrorMask = Buffer[F + 2];
            Scheme = Buffer[F + 3];
          }
          //*******************  Frame 4  *******************
          F = FOffset + FLength * 3;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            Relay2 = (Buffer[F]);
            OperatingHoursRelay2 = Buffer[F + 2] + Buffer[F + 3] * 256;
          }
          //*******************  Frame 5  *******************
          F = FOffset + FLength * 4;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
          }
          //*******************  Frame 6  *******************
          F = FOffset + FLength * 5;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            SystemTime = Buffer[F + 3] << 8 | Buffer[F + 2];
          }
          //*******************  Frame 7  *******************
          F = FOffset + FLength * 6;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
          }
          //*******************  Frame 8  *******************
          F = FOffset + FLength * 7;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            HeatQuantity = Buffer[F] + Buffer[F + 1] * 256 + Buffer[F + 2] * 65536 + Buffer[F + 3] * 16777216;
          }
          ///******************* End of frames ****************

        } // end 0x2211 DeltaSol CS Plus or 0x4211 DeltaSol BS Plus
        else if (Source_address == 0x7311) { // Deltasol M, alias Roth B/W Komfort
          // 6 temp frames, 12 sensors
          // Only decoding the first four due to library limitations
          unsigned int frame = 0;
          // Frame 1:
          F = FOffset + frame * FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor1Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor2Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          // Frame 2:
          frame++;
          F = FOffset + frame * FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor3Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor4Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          // Frame 7: Irradiation and unused
          /*
          frame = 7
          F = FOffset + 6 * FLength;
          irradiation = CalcTemp(Buffer[F + 1], Buffer[F]);
          */
          // Frame 8: Pulse counter 1
          // Frame 9: Pulse counter 2
          // Frame 10: Sensor errors: no sensor / shorted
          // Frame 11: Sensors
          // Frame 12: Relays 1-4
          // Frame 13: Relays 5-8
          // Frame 14: Relays 9-12
          frame = 11;
          F = FOffset + frame * FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Relay1 = Buffer[F];
            Relay2 = Buffer[F + 1];
          }
          // Frame 15: Not used / relays
          // Frame 16: Errors / warnings
          // Frame 17: Version, revision / time
          #ifdef DEBUG
          Serial.println("Got values");
          Serial.print("Temperature: ");
          Serial.print(sensor1Temp);
          Serial.print(", ");
          Serial.print(sensor2Temp);
          Serial.print(". Relays: ");
          Serial.print(Relay1);
          Serial.print(", ");
          Serial.println(Relay2);
          #endif
        }

        /* Add your own controller ID and code in the if statement below and uncomment
          else if (Source_address ==0x????){
          }
        */
        else {

          // Default temp 1-4 extraction
          // For most Resol controllers temp 1-4 are always available, so
          // even if you do not know the datagram format you can still see
          // these temps 1 to 4.

          //

          //Offset  Size    Mask    Name                    Factor  Unit
          // Frame 1
          //0       2               Temperature sensor 1    0.1     &#65533;C
          //2       2               Temperature sensor 2    0.1     &#65533;C
          // Frame 2
          //4       2               Temperature sensor 3    0.1     &#65533;C
          //6       2               Temperature sensor 4    0.1     &#65533;C
          //
          // Each frame has 6 bytes
          // byte 1 to 4 are data bytes -> MSB of each bytes
          // byte 5 is a septet and contains MSB of bytes 1 to 4
          // byte 6 is a checksum
          //
          //*******************  Frame 1  *******************

          F = FOffset;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            // 'collector1' Temperatur Sensor 1, 15 bits, factor 0.1 in C
            sensor1Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            // 'store1' Temperature sensor 2, 15 bits, factor 0.1 in C
            sensor2Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          //*******************  Frame 2  *******************
          F = FOffset + FLength;
          if (Buffer[F + 5] == VBus_CalcCrc(Buffer, F, 5)) { // CRC ok
            Septet = Buffer[F + FSeptet];
            InjectSeptet(Buffer, F, 4);
            sensor3Temp = calcTemp(Buffer[F + 1], Buffer[F]);
            sensor4Temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);
          }
          ///******************* End of frames ****************

        } //End of Default temp 1-4 extraction

        if (sensor1Temp > sensor1TempMax)
          sensor1TempMax = sensor1Temp;
        if (sensor2Temp > sensor2TempMax)
          sensor2TempMax = sensor2Temp;
        if (sensor3Temp > sensor3TempMax)
          sensor3TempMax = sensor3Temp;
        if (sensor4Temp > sensor4TempMax)
          sensor4TempMax = sensor4Temp;

      } // end if command 0x0100
    } // end !quit

    return !quit;
  } // end VBusRead()
  // This function converts 2 data bytes to a temperature value.
  float calcTemp(int Byte1, int Byte2) {
    int v;
    v = Byte1 << 8 | Byte2; //bit shift 8 to left, bitwise OR

    if (Byte1 == 0x00) {
      v = v & 0xFF;
    }

    if (Byte1 == 0xFF)
      v = v - 0x10000;

    if (v == SENSORNOTCONNECTED)
      v = 0;

    return (float)((float) v * 0.1);
  }

  // The following is needed for decoding the data
  void InjectSeptet(unsigned char * Buffer, int Offset, int Length) {
    for (unsigned int i = 0; i < Length; i++) {
      if (Septet & (1 << i)) {
        Buffer[Offset + i] |= 0x80;
      }
    }

    PrintHex8( & Buffer[Offset], Length);
  }

  // CRC calculation
  // From https://danielwippermann.github.io/resol-vbus/vbus-specification.html
  unsigned char VBus_CalcCrc(unsigned char * Buffer, int Offset, int Length) {
    unsigned char CRC = 0x7F;

    for (int i = 0; i < Length; i++) {
      CRC = (CRC - Buffer[Offset + i]) & 0x7F;
    }
    return CRC;
    // Prints the hex values of the char array sent to it.
  }
  void PrintHex8(unsigned char * data, uint8_t length) // prints 8-bit data in hex with leading zeroes
  {
    #if DEBUG
    Serial.print("0x");
    for (int i = 0; i < length; i++) {
      if (data[i] < 0x10) {
        Serial.print("0");
      }
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }

    Serial.println();
    #endif
  }



};
