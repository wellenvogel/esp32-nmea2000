Sensors
=======
The software contains support for a couple of sensors (starting from [20231228](../../releases/tag/20231228) and extend in consecutive releases).
This includes some I2C Sensors and SSI rotary encoders.
To connect sensors the following steps are necessary:

1. Check for the supported sensors and decide which base you would like to use. For i2c sensors you can typically connect a couple of them to one i2c bus - so the M5 grove connector would suit this well. If you are not directly using the M5 modules you may need to check the voltages: The M5 grove carries 5V but the logic levels are 3.3V.
Check e.g. the [description of the M5 atom lite](https://docs.m5stack.com/en/core/atom_lite) for the pinout of the grove port.

2. Use the [online build service](BuildService.md) to create a binary that matches your configuration and flash it.

3. Use the configuration to fine tune the parameters for the NMEA2000 messages and the NMEA0183 messages. Potentially you also can adjust some calibration values.

Configuration and Measure Flow
------------------------------
During the building of the binary a couple of compile flags will control which buses (i2c, spi) and which sensors will be built into the binary.
On startup the software will try to initialize the sensors and will start the periodic measurement tasks.
Measured sensor data will always be sent as NMEA2000. For each sensor you can additionally define some mapping to NMEA0183 XDR transducer values (if there is no native NMEA0183 record for them).
To ensure that the sensor data will be shown at the "data" page this conversion is necessary (i.e. if you disable this conversion sensor data will still be sent to the NMEA2000 bus - but will not be shown at the data page).
For each sensor you can separately enable and disable it in the configuration.
You can also typically select the NMEA2000 instance id for the sensor value and the measurement interval.

Implementation
--------------
Sensors are implemented in [iictask](../lib/iictask) and in [spitaks](../lib/spitask/).
They are implemented as [usertasks](../lib/exampletask/Readme.md).

Bus Usage
---------
When selecting sensors to be connected at the M5 grove ports in the [online build service](BuildService.md) the system will select the appropriate bus (i2c-1, i2c-2) by it's own. As you can have up to 4 grove ports (one at the device and 3 by using the [M5 Atomic PortABC](https://shop.m5stack.com/products/atomic-portabc-extension-base)) you can use both available i2c buses (and still utilize other groves for serial or CAN).

Implemented Sensors
-------------------
* [BME280](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf): temperature/humidity/pressure [PGNs: 130314,130312, 130313]
* [QMP6988](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/enviii/QMP6988%20Datasheet.pdf): pressure [PGN: 130314]
* [SHT30](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/SHT3x_Datasheet_digital.pdf): temperature and humidity [PGNs: 130312, 130313]
* [M5-ENV3](https://docs.m5stack.com/en/unit/envIII): combination of QMP6988 and SHT30 [PGNs: 130314,130312, 130313]
* [DMS22B](https://www.mouser.de/datasheet/2/54/bour_s_a0011704065_1-2262614.pdf)[since 20240324]: SSI rotary encoder (needs level shifters to / from 5V!) [PGN: 127245]
