Serial and USB
==============
The gateway software uses the [arduino layer](https://github.com/espressif/arduino-esp32) on top of the [espressif IDF framework](https://github.com/espressif/esp-idf).
The handling of serial devices is mainly done by the implementations in the arduino-espressif layer.
The gateway code adds some buffering on top of this implementation and ensures that normally only full nmea records are sent.
If the external connection is to slow the gateway will drop complete records.
All handling of the serial channels is done in the main task of the gateway. 

Serial Devices
--------------
THe arduino espressif layer provides the serial devices as [Streams](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Stream.h#L48).
Main implementations are [HardwareSerial](https://github.com/espressif/arduino-esp32/blob/3670e2bf2aca822f2e1225fdb0e0796e490005a8/cores/esp32/HardwareSerial.h#L71) - for the UARTS and [HWCDC](https://github.com/espressif/arduino-esp32/blob/3670e2bf2aca822f2e1225fdb0e0796e490005a8/cores/esp32/HWCDC.h#L43)(C3/S3 only) - for the USB CDC hardware device.

For the github versions: arduino-espressif 3.20009 maps to github tag 2.0.9.

The arduino espressif layer creates a couple of global instances for the serial devices (potentially depending on some defines).
The important defines for C3/S3 are:

 * ARDUINO_USB_MODE - the gateway always expects this to be 1
 * ARDUINO_USB_CDC_ON_BOOT - 0 or 1 (CB in the table below)

The created devices for framework 3.20009:

| Device(Variable) | Type(ESP32) | C3/S3 CB=0 | C3/S3 CB=1 |
| ------------ | ------- | ------ | -----|
| Serial0 | --- | --- | HardwareSerial(0) |
| Serial1 | HardwareSerial(1) | HardwareSerial(1) | HardwareSerial(1) |
| Serial2 | HardwareSerial(2) | HardwareSerial(2) | HardwareSerial(2) |
| USBSerial | --- | HWCDC | ---- | 
| Serial | HardwareSerial(0) | HardwareSerial(0) | HWCDC | 

Unfortunately it seems that in newer versions of the framework the devices could change.

The gateway will use the following serial devices:

* USBserial:<br> 
  For debug output and NMEA as USB connection. If you do not use an S3/C3 with ARDUINO_USB_CDC_ON_BOOT=0 you need to add a <br>_define USBSerial Serial_ somewhere in your build flags or in your task header.<br>
  Currently the gateway does not support setting the pins for the USB channel (that would be possible in principle only if an external PHY device is used and the USB is connected to a normal UART).
* Serial1:<br>
  If you define GWSERIAL_TYPE (1,2,3,4) - see [defines](../lib/hardware/GwHardware.h#23) or GWSERIAL_MODE ("UNI","BI","TX","RX") it will be used for the first serial channel.
* Serial2:<br>
  If you define GWSERIAL2_TYPE (1,2,3,4) - see [defines](../lib/hardware/GwHardware.h#23) or GWSERIAL2_MODE ("UNI","BI","TX","RX") it will be used for the second serial channel.

Hints
-----
For normal ESP32 chips you need to set <br>_define USBSerial Serial_ <br>and you can use up to 2 serial channels beside the USB channel.
For C3/S3 chips you can go for 2 options:
1. set ARDUINO_USB_CDC_ON_BOOT=1: in This case you still need to set<br> _define USBSerial Serial_<br> You can use up to 2 serial channels in the gateway core - but you still have Serial0 available for a third channel (not yet supported by the core - but can be used in your user code)
2. set ARDUINO_USB_CDC_ON_BOOT=0: in this case USBSerial is already defined as the USB channel. You can use 2 channels in the gateway core and optional you can use Serial in your user code.

If you do not set any of the GWSERIAL* defines (and they are not set by the core hardware definitions) you can freely use Serial1 and / or Serial2 in your user code.
