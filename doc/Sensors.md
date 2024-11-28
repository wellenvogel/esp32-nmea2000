Sensors
=======
The software contains support for a couple of sensors (starting from [20231228](../../releases/tag/20231228) and extended in consecutive releases).
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

Implementing Own Sensors
---------------------
To add an own sensor implementation you typically need to handle the following parts:
* (opt) add a library that supports your sensor
* add some [XDR Mapping](./XdrMappings.md) that will convert your generated NMEA2000 message into an NMEA0183 XDR record and ensure the display on the data page
* implement the sensor initialization
* implement the measurement and generating the NMEA2000 message

You typically would do this in a [user task](../lib/exampletask/Readme.md).<br>
You can either just implement everything by your own or reuse the existing infrastructure for sensors.

OwnImplementation
__________________

To implement everything by your own just create a config.json for the parameters you need, add an XDR mapping in a task init function (see e.g. [PressureXdr](../lib/iictask/GwIicSensors.h#L27)).
In your user taks just initialize the sensor using your config values and add a loop that periodically measures the sensor value and sends out an nmea2000 message (using the [api->sendN2KMessage](../lib/api/GwApi.h#L137)).
To display some information on the status page just add a countergroup in your task init function ([api->addCounter](../lib/api/GwApi.h#L170)) and increment a counter of this group on every measure ([api->increment](../lib/api/GwApi.h#L171)).
To utilize a bus you typically would need to add the related library to your environment and add some bus initialization to define the pins that are used for this particular bus.
Be carefull if you additionally would like to use sensors from the core as the core potentially would already initialize some bus - depending on the compile flags you provide.
If you need additional libraries for your sensor just add a platformio.ini to your usertask and define an environment that contains the additional libraries.
If you would like to compile also other environments (i.e. without the additional libraries) you should wrap all the code that references the additional libraries with some #ifdef and add a define to your environment (see the implementations of the [sensors in the core](../lib/iictask/)).

Using the core infrastructure
_____________________________
For sensors of bus types that are already supported by the core (mainly I2C) you can simplify your implementation.
Just also start with a [usertask](../lib/exampletask/Readme.md). But you only need the task init function, a config.json and potentially a platformio.ini.
In your task code just implement a class that handles the sensor - it should inherit from [SensorBase](../lib/sensors/GwSensor.h#L20) or from [IICSensorBase](../lib/iictask/GwIicSensors.h#L16).<br>
You need to implement:
* _readConfig_ - just read your configuration and fill attributes of your class. Especially set the "ok" to true and fill the interval field to define your measure interval.
* _preinit_ - check if your snesor is configured ,add necessary XDR mappings and return true if your sensor is active. Do __not__ yet initialize the sensor hardware.
* _isActive_ - return true if your sensor is active
* _initDevice_ - init your sensor hardware and return true if this was ok
* _measure_ - read the sensor data, send NMEA2000 messages and increment 
counters

The busType and busId fields of your imnplementation have to be set correctly.<br>
In your task init function add the sensors you would like to be handled using [api->addSensor](../lib/api/GwApi.h#L218).

All the internal sensors are implemented using this approach - e.g. [BME280](../lib/iictask/GwBME280.cpp#L23).<br>
Do not get confused by all the different defines and the special config handling - this is only there to be as generic as possible - typically not necessary for your own sensor implementation.

To use an IIC bus you need to compile with flags that define the pins to be used for the IIC bus:
* busId 1 (IIC bus 1): GWIIC_SDA, GWIIC_SCL
* busId 2 (IIC bus 2): GWIIC_SDA2, GWIIC_SCL2

So you would need to add such definitions to your environment in your platformio.ini.

Implemented Sensors
-------------------
* [BME280](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf): temperature/humidity/pressure [PGNs: 130314,130312, 130313, 130311 since 20241128 ]
* [BMP280](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf) [since 20241128]: temperature/pressure [PGNs: 130314,130312, 130311]
* [QMP6988](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/enviii/QMP6988%20Datasheet.pdf): pressure [PGN: 130314]
* [SHT30](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/SHT3x_Datasheet_digital.pdf): temperature and humidity [PGNs: 130312, 130313]
* [M5-ENV3](https://docs.m5stack.com/en/unit/envIII): combination of QMP6988 and SHT30 [PGNs: 130314,130312, 130313]
* [DMS22B](https://www.mouser.de/datasheet/2/54/bour_s_a0011704065_1-2262614.pdf)[since 20240324]: SSI rotary encoder (needs level shifters to / from 5V!) [PGN: 127245]
