# vbus-arduino-library
Library to allow your program to readout of VBus interface Protocol 1.0 by Arduino.

## Goal:
Readout the Resol VBus interface on as many VBUS devices as possible while presenting a common api/librray

## What does it do?
This sketch reads the VBus data and depending on the format of the controller decodes the data and puts it in variables.

## Is it stable?
To be seen.

## Controller support
Currently supports the following controllers:
* Resol DeltaTherm FK (0x5611)
* Oranier Aquacontrol III (0x5611)
* Conergy DT5 (0x3271)
* Joule / Resol Deltasol C (0x4212)

If it does not find any of the supported controllers, it will try to decode the first 2 frames which usually contain Temp 1 to 4.

## Hardware:
* VBus RX interface circuit
* Arduino Uno

VBus is NOT RS485. So you need a specific converter circuit to make the VBus data readable for the Arduino UART.
See f.i. [Here](https://piamble.wordpress.com/tag/vbus/).


AltSoftSerial is used for the VBus module.
Serial is used to debug the output to PC. 
Vbus serial works with 9600 Baudrate and 8N1.

Arduino Uno:
* Serial  on pins  8 (RX)


### My controller is not in the list, how can I add it?
Go to http://danielwippermann.github.io/resol-vbus/vbus-packets.html
and find your controller. In the list you can see which information the controller sends.
You need the controller ID and offset, bitsize and names of all the variables.
Now use the examples for the DT5 and FK in VBusRead() to create a new entry for your own controller.
This might be not that easy.
Do not forget to properly declare your new variables too.

If you have tested it and it works, please add a Pull request so I can integrate your controller here.

### Can the author add my controller?
No. First try it yourself. But if you fail, you can always ask.

### Is this sketch the pinnacle of proper programming?
Not by any means, but it works.
If you have any remark or improvement, let the author know.

#### Additional credits
Source code is based mainly on https://github.com/bbqkees/vbus-arduino-domoticz which in turn is based based on the VBus library from 'Willie' from the Mbed community.

#### Legal Notices
RESOL, VBus, VBus.net and others are trademarks or registered trademarks of RESOL - Elektronische Regelungen GmbH.

All other trademarks are the property of their respective owners.
