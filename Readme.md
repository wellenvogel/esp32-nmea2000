NMEA2000-Gateway with ESP32
===========================

Based on the work of 
* [Homberger](https://github.com/AK-Homberger/NMEA2000WifiGateway-with-ESP32) 
* [Timo Lappalainen](https://github.com/ttlappalainen/NMEA2000)
* [Arno Duvenhage](https://github.com/aduvenhage/ais-decoder)
and a couple of other open source projects.
Many thanks for all the great work.

Goal
----
Have a simple ready-to-go ESP32 binary that can be flashed onto a [M5 Atom CAN](https://docs.m5stack.com/en/atom/atom_can), potentially extended by an [Atom Tail485](https://shop.m5stack.com/collections/atom-series/products/atom-tail485?variant=32169041559642) for NMEA0183 connection and power supply.

But will also run on other ESP32 boards.

Modes
-----
* NMEA2000 -> Wifi (NMEA0183)
* NMEA2000 -> USB (NMEA0183)
* NMEA0183 -> Wifi
* NMEA0183 -> NMEA2000
* Wifi (NMEA0183) -> NMEA2000
* USB (NMEA0183) -> NMEA2000
* ....

Environment
-----------
[PlatformIO](https://platformio.org/).
Should be possible to use [M5Burner](https://docs.m5stack.com/en/download) to flash ready binaries.

Pre Build Binaries
------------------
In the [release section](https://github.com/wellenvogel/esp32-nmea2000/releases) you can find a couple of pre-build binaries that can easily be flashed on your ESP32 board using [ESPTool](https://github.com/espressif/esptool).
The flash command must be (example for m5stack-atom):

```
esptool.py --port XXXX --chip esp32 write_flash 0x1000  m5stack-atom-all.bin
```
For the meaning of the boar names have a look in [platformio.ini](platformio.ini) and look for the hardware definitions in [GwHardware.h](lib/hardware/GwHardware.h).

Starting
---------
After flushing a wifi access point is created. Connect to it (name: ESP32NMEA2K, password: esp32nmea2k).
Afterwards use a Bonjour Browser, the address ESP32NMEA2k.local or the ip address 192.168.15.1 to connect with your browser.
You will get a small UI to watch the status and make settings.
If you want to connect to another wifi network, just enter the credentials in the wifi client tab.
On the data page you will have a small dashboard for the currently received data.
On the status page you can check the number of messages flowing in and out.

more to come...



