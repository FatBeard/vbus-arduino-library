#include "VBUSDecoder.h"
#define DEBUG 0

// Clear all maximum values
void VBUSDecoder::clearMaxValues()
{
  Sensor1_temp_max = 0.0;
  Sensor2_temp_max = 0.0;
  Sensor3_temp_max = 0.0;
  Sensor4_temp_max = 0.0;
}

bool VBUSDecoder::initialise()
{

  Serial1.begin(9600);
  clearMaxValues();
  return true;
} // end void initialise()

float VBUSDecoder::getS1Temp()
{

  return Sensor1_temp;
}

float VBUSDecoder::getS2Temp()
{
  return Sensor2_temp;
}

float VBUSDecoder::getS3Temp()
{
  return Sensor3_temp;
}

float VBUSDecoder::getS4Temp()
{
  return Sensor4_temp;
}

bool VBUSDecoder::getP1Status()
{
  return relayPump;
}

bool VBUSDecoder::getP2Status()
{
  return relay3WayValve;
}

int VBUSDecoder::getP1Speed()
{

  return String(Relay1, DEC).toInt();
}

int VBUSDecoder::getP2Speed()
{

  return String(Relay2, DEC).toInt();
}
int VBUSDecoder::getP1OperatingHours()
{

  return String(OperatingHoursRelais1, DEC).toInt();
}

int VBUSDecoder::getP2OperatingHours()
{

  return String(OperatingHoursRelais2, DEC).toInt();
}

bool VBUSDecoder::getAlertStatus()
{
  return SystemAlert;
}

String VBUSDecoder::getSystemTime()
{

  int hours = SystemTime / 60;
  int minutes = SystemTime % 60;
  String toReturn = String(String(hours) + ":" + String(minutes));

  if (hours < 10)
  {
    toReturn = "0" + toReturn;
  }

  return toReturn;
}

bool VBUSDecoder::readSensor()
{

  if (vBusRead())
  {
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
    Serial.println(Sensor1_temp);
    Serial.print("Sensor 2: ");
    Serial.println(Sensor2_temp);
    Serial.print("Sensor 3: ");
    Serial.println(Sensor3_temp);
    Serial.print("Sensor 4: ");
    Serial.println(Sensor4_temp);
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
  if (Relay1 == 0x64)
  {
    relayPump = true;
  }
  else if (Relay1 == 0x00)
  {
    relayPump = false;
  }
  else
  {
    relayPump = false;
  }

  if (Relay2 == 0x64)
  {
    relay3WayValve = true;
  }
  else if (Relay2 == 0x00)
  {
    relay3WayValve = false;
  }
  else
  {
    relay3WayValve = false;
  }

  if (SystemNotification != 0x00 || ErrorMask != 0x00) // Not really sure what ErrorMask is, treating as system alert.
  {
    SystemAlert = true;
  }
  else
  {
    SystemAlert = false;
  }

  return true;
}

// The following is needed for decoding the data
void VBUSDecoder::InjectSeptet(unsigned char *Buffer, int Offset, int Length)
{
  for (unsigned int i = 0; i < Length; i++)
  {
    if (Septet & (1 << i))
    {
      Buffer[Offset + i] |= 0x80;
    }
  }
}

// The following function reads the data from the bus and converts it all
// depending on the used VBus controller.
bool VBUSDecoder::vBusRead()
{
  int F;
  char c;
  bool start, stop, quit;

  start = true;
  stop = false;
  quit = false;
  Bufferlength = 0;
  lastTimeTimer = 0;
  lastTimeTimer = millis();

  while ((!stop) and (!quit))
  {
    if (Serial1.available())
    {
      c = Serial1.read();

      char sync1 = 0xAA;
      if (c == sync1)
      {

#if DEBUG
// Serial.println("Sync found");
#endif

        if (start)
        {
          start = false;
          Bufferlength = 0;
          //#if DEBUG
          //#endif
        }
        else
        {
          if (Bufferlength < 20)
          {
            lastTimeTimer = 0;
            lastTimeTimer = millis();
            Bufferlength = 0;
          }
          else
            stop = true;
        }
      }
#if DEBUG
// Serial.println(c, HEX);
#endif
      if ((!start) and (!stop))
      {
        Buffer[Bufferlength] = c;
        Bufferlength++;
      }
    }
    if ((timerInterval > 0) && (millis() - lastTimeTimer > timerInterval))
    {
      quit = true;
#if DEBUG
//   Serial.print("Timeout: ");
//   Serial.println(lastTimeTimer);
#endif
    }
  }

  lastTimeTimer = 0;

  if (!quit)
  {
    Destination_address = Buffer[2] << 8;
    Destination_address |= Buffer[1];
    Source_address = Buffer[4] << 8;
    Source_address |= Buffer[3];
    ProtocolVersion = (Buffer[5] >> 4) + (Buffer[5] & (1 << 15));

    Command = Buffer[7] << 8;
    Command |= Buffer[6];
    Framecnt = Buffer[8];
    Checksum = Buffer[9]; //TODO check if Checksum is OK
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

    if ((Command == 0x0100) and (Bufferlength == 10 + Framecnt * 6))
    {

      //Only decode the data from the correct source address
      //(There might be other VBus devices on the same bus).

      if (Source_address == 0x3271)
      {
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

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        // 'collector1' Temperatur Sensor 1, 15 bits, factor 0.1 in C
        Sensor1_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        // 'store1' Temperature sensor 2, 15 bits, factor 0.1 in C
        Sensor2_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        //*******************  Frame 2  *******************
        F = FOffset + FLength;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        Sensor3_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        Sensor4_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        //*******************  Frame 3  *******************
        F = FOffset + FLength * 2;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        PumpSpeed1 = (Buffer[F]);
        PumpSpeed2 = (Buffer[F + 1]);
        RelaisMask = Buffer[F + 2];
        ErrorMask = Buffer[F + 3];

        //*******************  Frame 4  *******************
        F = FOffset + FLength * 3;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        SystemTime = Buffer[F + 1] << 8 | Buffer[F];
        Scheme = Buffer[F + 2];

        OptionPostPulse = (Buffer[F + 3] & 0x01);
        OptionThermostat = ((Buffer[F + 3] & 0x02) >> 1);
        OptionHQM = ((Buffer[F + 3] & 0x04) >> 2);

        //*******************  Frame 5  *******************
        F = FOffset + FLength * 4;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        OperatingHoursRelais1 = Buffer[F + 1] << 8 | Buffer[F];
        OperatingHoursRelais2 = Buffer[F + 3] << 8 | Buffer[F + 2];

        //*******************  Frame 6  *******************
        F = FOffset + FLength * 5;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        HeatQuantity = (Buffer[F + 1] << 8 | Buffer[F]) + (Buffer[F + 3] << 8 | Buffer[F + 2]) * 1000;

        //*******************  Frame 7  *******************
        F = FOffset + FLength * 6;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        HeatQuantity = HeatQuantity + (Buffer[F + 1] << 8 | Buffer[F]) * 1000000;
        Version = Buffer[F + 3] << 8 | Buffer[F + 2];

        ///******************* End of frames ****************

      } // end 0x3271 Conenergy DT5

      else if (Source_address == 0x5611)
      {
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

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        Sensor1_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        Sensor2_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        //*******************  Frame 2  *******************
        F = FOffset + FLength;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        Sensor3_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        Sensor4_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        //*******************  Frame 3  *******************
        F = FOffset + FLength * 2;

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
        //*******************  Frame 4  *******************
        F = FOffset + FLength * 3;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        // System date is not needed for Domoticz

        //*******************  Frame 5  *******************
        F = FOffset + FLength * 4;

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

        ///******************* End of frames ****************

      } //End 0x5611 Resol DeltaTherm FK

      else if (Source_address == 0x4212)
      {

#if DEBUG
        Serial.println("---------------");
        Serial.println("Now decoding for DeltaSol C 0x4212");
        Serial.println("---------------");

#endif

        // Frame info for the Resol DeltaSol C (Joule)
        // check VBusprotocol specification for other products

        // This library is made for the Resol DeltaSol C (0x4212)

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
        //9                   Pump Speed R2       1           %
        //10                  Error Mask          1
        //11                  Scheme              1
        //12                  Operating Hours R1  1           h
        //13                  Operating Hours R1  256         h
        //14                  Operating Hours R2  1           h
        //15                  Operating Hours R2  256         h
        //16                  Heat Quantity       1           Wh
        //17                  Heat Quantity       256         Wh
        //18                  Heat Quantity       1000        Wh
        //19                  Heat Quantity       256000      Wh
        //20                  Heat Quantity       1000000     Wh
        //21                  Heat Quantity       256000000   Wh
        //22                  Minute of Day       1
        //23                  Minute of Day       256

        //
        // Each frame has 6 bytes
        // byte 1 to 4 are data bytes -> MSB of each bytes
        // byte 5 is a septet and contains MSB of bytes 1 to 4
        // byte 6 is a checksum
        //
        //*******************  Frame 1  *******************

        F = FOffset;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        // 'collector1' Temperatur Sensor 1, 15 bits, factor 0.1 in C
        Sensor1_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        // 'store1' Temperature sensor 2, 15 bits, factor 0.1 in C
        Sensor2_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        PrintHex8(&Buffer[F], 4);

        //*******************  Frame 2  *******************
        F = FOffset + FLength;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        Sensor3_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        Sensor4_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        PrintHex8(&Buffer[F], 4);

        //*******************  Frame 3  *******************
        F = FOffset + FLength * 2;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        Relay1 = (Buffer[F]);
        Relay2 = (Buffer[F + 1]);
        ErrorMask = Buffer[F + 2];
        Scheme = Buffer[F + 3];

        PrintHex8(&Buffer[F], 4);

        //*******************  Frame 4  *******************
        F = FOffset + FLength * 3;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        OperatingHoursRelais1 = Buffer[F + 1] << 8 | Buffer[F];
        OperatingHoursRelais2 = Buffer[F + 3] << 8 | Buffer[F + 2];

        PrintHex8(&Buffer[F], 4);

        //*******************  Frame 5  *******************
        F = FOffset + FLength * 4;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        HeatQuantity = (Buffer[F + 1] << 8 | Buffer[F]) + (Buffer[F + 3] << 8 | Buffer[F + 2]) * 1000;

        PrintHex8(&Buffer[F], 4);

        //*******************  Frame 6  *******************
        F = FOffset + FLength * 5;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        HeatQuantity = HeatQuantity + (Buffer[F + 1] << 8 | Buffer[F]) * 1000000;

        SystemTime = Buffer[F + 3] << 8 | Buffer[F + 2];

        PrintHex8(&Buffer[F], 4);

        //*******************  Frame 7  *******************
        F = FOffset + FLength * 6;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        PrintHex8(&Buffer[F], 4);

        ///******************* End of frames ****************

      } // end 0x4212 DeltaSol C

      /* Add your own controller ID and code in the if statement below and uncomment
        else if (Source_address ==0x????){
        }
      */

      else
      {

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

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        // 'collector1' Temperatur Sensor 1, 15 bits, factor 0.1 in C
        Sensor1_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        // 'store1' Temperature sensor 2, 15 bits, factor 0.1 in C
        Sensor2_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        //*******************  Frame 2  *******************
        F = FOffset + FLength;

        Septet = Buffer[F + FSeptet];
        InjectSeptet(Buffer, F, 4);

        Sensor3_temp = calcTemp(Buffer[F + 1], Buffer[F]);
        Sensor4_temp = calcTemp(Buffer[F + 3], Buffer[F + 2]);

        ///******************* End of frames ****************

      } //End of Default temp 1-4 extraction

      if (Sensor1_temp > Sensor1_temp_max)
        Sensor1_temp_max = Sensor1_temp;
      if (Sensor2_temp > Sensor2_temp_max)
        Sensor2_temp_max = Sensor2_temp;
      if (Sensor3_temp > Sensor3_temp_max)
        Sensor3_temp_max = Sensor3_temp;
      if (Sensor4_temp > Sensor4_temp_max)
        Sensor4_temp_max = Sensor4_temp;

    } // end if command 0x0100
  }   // end !quit

  return !quit;
} // end VBusRead()

// This function converts 2 data bytes to a temperature value.
float VBUSDecoder::calcTemp(int Byte1, int Byte2)
{
  int v;
  v = Byte1 << 8 | Byte2; //bit shift 8 to left, bitwise OR

  if (Byte1 == 0x00)
  {
    v = v & 0xFF;
  }

  if (Byte1 == 0xFF)
    v = v - 0x10000;

  if (v == SENSORNOTCONNECTED)
    v = 0;

  return (float)((float)v * 0.1);
}

void VBUSDecoder::PrintHex8(unsigned char *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
#if DEBUG
  Serial.print("0x");
  for (int i = 0; i < length; i++)
  {
    if (data[i] < 0x10)
    {
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }

  Serial.println();
#endif
}