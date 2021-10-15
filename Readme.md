NMEA2000-Gateway with ESP32
===========================

Based on the work of [Homberger](https://github.com/AK-Homberger/NMEA2000WifiGateway-with-ESP32) - many thanks...

Goal
----
Have a simple ready-to-go ESP32 binary that can be flashed onto a [M5 Atom CAN](https://docs.m5stack.com/en/atom/atom_can), potentially extended by an [Atom Tail485](https://shop.m5stack.com/collections/atom-series/products/atom-tail485?variant=32169041559642) for NMEA0183 connection and power supply

Modes
-----
NMEA2000 -> Wifi (NMEA0183)
NMEA0183 -> Wifi
NMEA0183 -> NMEA2000
Wifi (NMEA0183) -> NMEA2000
....

Environment
-----------
[PlatformIO](https://platformio.org/).
Should be possible to use [M5Burner](https://docs.m5stack.com/en/download) to flash ready binaries.




