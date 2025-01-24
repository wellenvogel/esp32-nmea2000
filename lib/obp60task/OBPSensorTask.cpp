#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3
#include <Adafruit_Sensor.h>            // Adafruit Lib for sensors
#include <Adafruit_BME280.h>            // Adafruit Lib for BME280
#include <Adafruit_BMP280.h>            // Adafruit Lib for BMP280
#include <Adafruit_BMP085.h>            // Adafruit Lib for BMP085 and BMP180
#include <HTU21D.h>                     // Lib for SHT21/HTU21
#include "AS5600.h"                     // Lib for magnetic rotation sensor AS5600
#include <INA226.h>                     // Lib for power management IC INA226
#include <Ticker.h>                     // Timer Lib for timer
#include <RTClib.h>                     // DS1388 RTC
#include <OneWire.h>                    // 1Wire Lib
#include <DallasTemperature.h>          // Lib for DS18B20
#include "OBPSensorTask.h"              // Lib for sensor reading
#include "OBP60Hardware.h"              // Hardware definitions
#include "N2kMessages.h"                // Lib for NMEA2000
#include "NMEA0183.h"                   // Lib for NMEA0183
#include "ObpNmea0183.h"                // Check NMEA0183 sentence for uncorrect content
#include "OBP60Extensions.h"            // Lib for hardware extensions
#include "movingAvg.h"                  // Lib for moving average building

// Timer for hardware functions
Ticker Timer1(blinkingFlashLED, 500);   // Satrt Timer1 for flash LED all 500ms

// Initialization for all sensors (RS232, I2C, 1Wire, IOs)
//####################################################################################

void sensorTask(void *param){
    SharedData *shared = (SharedData *)param;
    GwApi *api = shared->api;
    GwLog *logger = api->getLogger();
    LOG_DEBUG(GwLog::LOG, "Sensor task started");
    SensorData sensors;
    ObpNmea0183 NMEA0183;

    Adafruit_BME280 bme280;         // Evironment sensor BME280
    Adafruit_BMP280 bmp280;         // Evironment sensor BMP280
    Adafruit_BMP085 bmp085;         // Evironment sensor BMP085 and BMP180
    HTU21D sht21(HTU21D_RES_RH12_TEMP14); // Environment sensor SHT21 and HTU21
    AMS_5600 as5600;                // Rotation sensor AS5600
    INA226 ina226_1(INA226_I2C_ADDR1);// Power management sensor INA226 Battery
    INA226 ina226_2(INA226_I2C_ADDR2);// Power management sensor INA226 Solar
    INA226 ina226_3(INA226_I2C_ADDR3);// Power management sensor INA226 Generator
    RTC_DS1388 ds1388;              // RTC DS1388
    OneWire oneWire(OBP_1WIRE);     // 1Wire bus
    DallasTemperature ds18b20(&oneWire);// Sensors for DS18B20
    DeviceAddress tempDeviceAddress;// Table for DS18B20 device addresses 

    // Init sensor stuff
    bool oneWire_ready = false;     // 1Wire initialized and ready to use
    bool RTC_ready = false;         // DS1388 initialized and ready to use
    bool GPS_ready = false;         // GPS initialized and ready to use
    bool BME280_ready = false;      // BME280 initialized and ready to use
    bool BMP280_ready = false;      // BMP280 initialized and ready to use
    bool BMP180_ready = false;      // BMP180 initialized and ready to use
    bool SHT21_ready = false;       // SHT21 initialized and ready to use
    bool AS5600_ready = false;      // AS5600 initialized and ready to use
    bool INA226_1_ready = false;    // INA226_1 initialized and ready to use
    bool INA226_2_ready = false;    // INA226_2 initialized and ready to use
    bool INA226_3_ready = false;    // INA226_3 initialized and ready to use

    // Create integer arrays for average building
    const int avgsize = 300;
    constexpr int arrayBatV{avgsize};
    constexpr int arrayBatC{avgsize};
    movingAvg batV(arrayBatV);
    movingAvg batC(arrayBatC);
    batV.begin();
    batC.begin();

    // Start timer
    Timer1.start(); // Start Timer1 for blinking LED

    // Direction settings for NMEA0183
    String nmea0183Mode = api->getConfig()->getConfigItem(api->getConfig()->serialDirection, true)->asString();
    api->getLogger()->logDebug(GwLog::LOG, "NMEA0183 Mode is: %s", nmea0183Mode.c_str());
    pinMode(OBP_DIRECTION_PIN, OUTPUT);
    if (String(nmea0183Mode) == "receive" || String(nmea0183Mode) == "off")
    {
        digitalWrite(OBP_DIRECTION_PIN, false);
    }
    if (String(nmea0183Mode) == "send")
    {
        digitalWrite(OBP_DIRECTION_PIN, true);
    }

    // Internal voltage sensor initialization
    String powsensor1 = api->getConfig()->getConfigItem(api->getConfig()->usePowSensor1, true)->asString();
    double voffset = (api->getConfig()->getConfigItem(api->getConfig()->vOffset,true)->asString()).toFloat();
    double vslope = (api->getConfig()->getConfigItem(api->getConfig()->vSlope,true)->asString()).toFloat();
    if(String(powsensor1) == "off"){
        #ifdef VOLTAGE_SENSOR
        sensors.batteryVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.53) * 2;   // Vin = 1/2 for OBP40
        #else
        sensors.batteryVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // Vin = 1/20 for OBP60    
        #endif
        sensors.batteryVoltage = sensors.batteryVoltage * vslope + voffset; // Calibration
        #ifdef LIPO_ACCU_1200
        sensors.BatteryChargeStatus = 0;    // Set to discharging
        sensors.batteryLevelLiPo = 0;       // Level 0...100%
        #endif
        sensors.batteryCurrent = 0;
        sensors.batteryPower = 0;
        // Fill average arrays with start values
        for (int i=1; i<=avgsize+1; ++i) {
            batV.reading(int(sensors.batteryVoltage * 100));
            batC.reading(int(sensors.batteryCurrent * 10));
        }
    }

    // Settings for 1Wire bus
    String oneWireOn=api->getConfig()->getConfigItem(api->getConfig()->useTempSensor,true)->asString();
    int numberOfDevices;
    if(String(oneWireOn) == "DS18B20"){
        ds18b20.begin();
        DeviceAddress tempDeviceAddress;
        numberOfDevices = ds18b20.getDeviceCount();
        // Limit for 8 sensors
        if(numberOfDevices > 8){
            numberOfDevices = 8;
        }
        if (numberOfDevices < 1) {
            oneWire_ready = false;
            api->getLogger()->logDebug(GwLog::ERROR,"Modul DS18B20 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"1Wire modul found at:");
            for(int i=0;i<numberOfDevices; i++){
                // Search the wire for address
                if(ds18b20.getAddress(tempDeviceAddress, i)){
                    api->getLogger()->logDebug(GwLog::LOG,"DS18B20-%01d: %12d", i, tempDeviceAddress[i]);
                } else {
                    api->getLogger()->logDebug(GwLog::LOG,"DS18B20-%01d: Sensor with errors, check wiring!", i);
                }
            }        
            oneWire_ready = true;
        }
    }

    // Settings for RTC
    String rtcOn=api->getConfig()->getConfigItem(api->getConfig()->useRTC,true)->asString();
    if(String(rtcOn) == "DS1388"){
        if (!ds1388.begin()) {
            RTC_ready = false;
            api->getLogger()->logDebug(GwLog::ERROR,"Modul DS1388 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul DS1388 found");
            uint year = ds1388.now().year();
            if(year < 2023){
                // ds1388.adjust(DateTime(__DATE__, __TIME__));  // Set date and time from PC file time
            }
            RTC_ready = true;
        }
    }

    // Settings for GPS sensors
    String gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asString();
    uint hdopAccuracy = uint(api->getConfig()->getConfigItem(api->getConfig()->hdopAccuracy,true)->asInt());
    if(String(gpsOn) == "NEO-6M"){
        Serial2.begin(9600, SERIAL_8N1, OBP_GPS_RX, OBP_GPS_TX, false); // not inverted (false)
        if (!Serial2) {     
            api->getLogger()->logDebug(GwLog::ERROR,"GPS modul NEO-6M not found, check wiring");
            GPS_ready = false;
            }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"GPS modul NEO-M6 found");
            NMEA0183.SetMessageStream(&Serial2);
            NMEA0183.Open();
            GPS_ready = true;
        }
    }
    if(String(gpsOn) == "NEO-M8N"){
        Serial2.begin(9600, SERIAL_8N1, OBP_GPS_RX, OBP_GPS_TX, false); // not inverted (false)
        if (!Serial2) {     
            api->getLogger()->logDebug(GwLog::ERROR,"GPS modul NEO-M8N not found, check wiring");
            GPS_ready = false;
            }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"GPS modul NEO-M8N found");
            NMEA0183.SetMessageStream(&Serial2);
            NMEA0183.Open();
            GPS_ready = true;
        }
    }
    if(String(gpsOn) == "ATGM336H"){
        Serial2.begin(9600, SERIAL_8N1, OBP_GPS_RX, OBP_GPS_TX, false); // not inverted (false)
        if (!Serial2) {     
            api->getLogger()->logDebug(GwLog::ERROR,"GPS modul ATGM336H not found, check wiring");
            GPS_ready = false;
            }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"GPS modul ATGM336H found");
            NMEA0183.SetMessageStream(&Serial2);
            NMEA0183.Open();
            GPS_ready = true;
        }
    }
    
    // Settings for environment sensors on I2C bus
    String envSensors=api->getConfig()->getConfigItem(api->getConfig()->useEnvSensor,true)->asString();

    if(String(envSensors) == "BME280"){
        if (!bme280.begin(BME280_I2C_ADDR)) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul BME280 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul BME280 found");
            sensors.airTemperature = bme280.readTemperature();
            sensors.airPressure = bme280.readPressure();
            sensors.airHumidity = bme280.readHumidity();
            BME280_ready = true;
        }
    }
    else if(String(envSensors) == "BMP280"){
        if (!bmp280.begin(BMP280_I2C_ADDR)) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul BMP280 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul BMP280 found");
            sensors.airTemperature = bmp280.readTemperature();
            sensors.airPressure = bmp280.readPressure();
            BMP280_ready = true;
        }
    }
    else if(String(envSensors) == "BMP085" || String(envSensors) == "BMP180"){
        if (!bmp085.begin()) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul BMP085/BMP180 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul BMP085/BMP180 found");
            sensors.airTemperature = bmp085.readTemperature();
            sensors.airPressure = bmp085.readPressure();
            BMP180_ready = true;
        }
    }
    else if(String(envSensors) == "HTU21" || String(envSensors) == "SHT21"){
        if (!sht21.begin()) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul HTU21/SHT21 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul HTU21/SHT21 found");
            sensors.airHumidity = sht21.readCompensatedHumidity();
            sensors.airTemperature = sht21.readTemperature();
            SHT21_ready = true;
        }
    }

    // Settings for rotation sensors AS5600 on I2C bus
    String envsensor = api->getConfig()->getConfigItem(api->getConfig()->useEnvSensor, true)->asString();
    String rotsensor = api->getConfig()->getConfigItem(api->getConfig()->useRotSensor, true)->asString();
    String rotfunction = api->getConfig()->getConfigItem(api->getConfig()->rotFunction, true)->asString();
    String rotSensor=api->getConfig()->getConfigItem(api->getConfig()->useRotSensor,true)->asString();
    
    if(String(rotSensor) == "AS5600"){
        Wire.beginTransmission(AS5600_I2C_ADDR);
        if (Wire.endTransmission() != 0) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul AS5600 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul AS5600 found");
            sensors.rotationAngle = DegToRad(as5600.getRawAngle() * 0.087);   // 0...4095 segments = 0.087 degree
            //sensors.magnitude = as5600.getMagnitude();              // Magnetic magnitude in [mT]
            AS5600_ready = true;
        }
    }

    // Settings for power amangement sensors INA226 #1 for Battery on I2C bus
    String shunt1 = api->getConfig()->getConfigItem(api->getConfig()->shunt1, true)->asString();
    // Settings for power amangement sensors INA226 #1 for Solar on I2C bus
    String powsensor2 = api->getConfig()->getConfigItem(api->getConfig()->usePowSensor2, true)->asString();
    String shunt2 = api->getConfig()->getConfigItem(api->getConfig()->shunt2, true)->asString();
    // Settings for power amangement sensors INA226 #1 for Generator on I2C bus
    String powsensor3 = api->getConfig()->getConfigItem(api->getConfig()->usePowSensor3, true)->asString();
    String shunt3 = api->getConfig()->getConfigItem(api->getConfig()->shunt3, true)->asString();

    float shuntResistor = 1.0;  // Default value for shunt resistor
    float maxCurrent = 10.0;    // Default value for max. current
    float corrFactor = 1;       // Correction factor for fix calibration
    
    // Battery sensor initialization
    if(String(powsensor1) == "INA226"){
        if (!ina226_1.begin()){
            api->getLogger()->logDebug(GwLog::ERROR,"Modul 1 INA226 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul 1 INA226 found");
            shuntResistor = SHUNT_VOLTAGE / float(shunt1.toInt());  // Calculate shunt resisitor for max. shunt voltage 75mV
            maxCurrent = shunt1.toFloat();
            api->getLogger()->logDebug(GwLog::LOG,"Calibation Modul 2 INA226, Imax:%3.0fA Rs:%7.5fOhm Us:%5.3f", maxCurrent, shuntResistor, SHUNT_VOLTAGE);
//            ina226_1.setMaxCurrentShunt(maxCurrent, shuntResistor);
            ina226_1.setMaxCurrentShunt(10, 0.01);  // Calibration with fix values (because the original values outer range)
            corrFactor = (maxCurrent / 10) * (0.001 / shuntResistor) / (maxCurrent / 100); // Correction factor for fix calibration
            sensors.batteryVoltage = ina226_1.getBusVoltage();
            sensors.batteryCurrent = ina226_1.getCurrent() * corrFactor;
            sensors.batteryPower = ina226_1.getPower() * corrFactor;
            // Fill average arrays with start values
            for (int i=1; i<=avgsize+1; ++i) {
                batV.reading(int(sensors.batteryVoltage * 100));
                batC.reading(int(sensors.batteryCurrent * 10));
            }
            INA226_1_ready = true;
        }
    }

    // Solar sensor initialization
    if(String(powsensor2) == "INA226"){
        if (!ina226_2.begin()){
            api->getLogger()->logDebug(GwLog::ERROR,"Modul 2 INA226 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul 2 INA226 found");
            shuntResistor = SHUNT_VOLTAGE / float(shunt2.toInt());  // Calculate shunt resisitor for max. shunt voltage 75mV
            maxCurrent = shunt2.toFloat();
            api->getLogger()->logDebug(GwLog::LOG,"Calibation Modul 2 INA226, Imax:%3.0fA Rs:%7.5fOhm Us:%5.3f", maxCurrent, shuntResistor, SHUNT_VOLTAGE);
//            ina226_1.setMaxCurrentShunt(maxCurrent, shuntResistor);
            ina226_2.setMaxCurrentShunt(10, 0.01);  // Calibration with fix values (because the original values outer range)
            corrFactor = (maxCurrent / 10) * (0.001 / shuntResistor) / (maxCurrent / 100); // Correction factor for fix calibration
            sensors.solarVoltage = ina226_2.getBusVoltage();
            sensors.solarCurrent = ina226_2.getCurrent() * corrFactor;
            sensors.solarPower = ina226_2.getPower() * corrFactor;
            // Fill average arrays with start values
            INA226_2_ready = true;
        }
    }

    // Generator sensor initialization
    if(String(powsensor3) == "INA226"){
        if (!ina226_3.begin()){
            api->getLogger()->logDebug(GwLog::ERROR,"Modul 3 INA226 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul 3 INA226 found");
            shuntResistor = SHUNT_VOLTAGE / float(shunt3.toInt());  // Calculate shunt resisitor for max. shunt voltage 75mV
            maxCurrent = shunt3.toFloat();
            api->getLogger()->logDebug(GwLog::LOG,"Calibation Modul 3 INA226, Imax:%3.0fA Rs:%7.5fOhm Us:%5.3f", maxCurrent, shuntResistor, SHUNT_VOLTAGE);
//            ina226_1.setMaxCurrentShunt(maxCurrent, shuntResistor);
            ina226_3.setMaxCurrentShunt(10, 0.01);  // Calibration with fix values (because the original values outer range)
            corrFactor = (maxCurrent / 10) * (0.001 / shuntResistor) / (maxCurrent / 100); // Correction factor for fix calibration
            sensors.generatorVoltage = ina226_3.getBusVoltage();
            sensors.generatorCurrent = ina226_3.getCurrent() * corrFactor;
            sensors.generatorPower = ina226_3.getPower() * corrFactor;
            // Fill average arrays with start values
            INA226_3_ready = true;
        }
    }

    int rotoffset = api->getConfig()->getConfigItem(api->getConfig()->rotOffset,true)->asInt();

    static long loopCounter = 0;    // Loop counter for 1Wire data transmission
    long starttime0 = millis();     // GPS update all 100ms
    long starttime5 = millis();     // Voltage update all 1s
    long starttime6 = millis();     // Environment sensor update all 1s
    long starttime7 = millis();     // Rotation sensor update all 500ms
    long starttime8 = millis();     // Battery power sensor update all 1s
    long starttime9 = millis();     // Solar power sensor update all 1s
    long starttime10 = millis();    // Generator power sensor update all 1s
    long starttime11 = millis();    // Copy GPS data to RTC all 5min
    long starttime12 = millis();    // Get RTC data all 500ms
    long starttime13 = millis();    // Get 1Wire sensor data all 2s

    tN2kMsg N2kMsg;
    shared->setSensorData(sensors); //set initially read values

    GwApi::BoatValue *gpsdays=new GwApi::BoatValue(GwBoatData::_GPSD);
    GwApi::BoatValue *gpsseconds=new GwApi::BoatValue(GwBoatData::_GPST);
    GwApi::BoatValue *hdop=new GwApi::BoatValue(GwBoatData::_HDOP);
    GwApi::BoatValue *valueList[]={gpsdays, gpsseconds, hdop};

    // Sensor task loop runs with 100ms
    //####################################################################################

    while (true){
        delay(100);                 // Loop time 100ms
        Timer1.update();            // Update for Timer2
        if (millis() > starttime0 + 100)
        {
            starttime0 = millis();
            // Send NMEA0183 GPS data on several bus systems all 100ms
            if (GPS_ready == true && hdop->value <= hdopAccuracy)
            {
                SNMEA0183Msg NMEA0183Msg;
                while (NMEA0183.GetMessageCor(NMEA0183Msg))
                {
                    api->sendNMEA0183Message(NMEA0183Msg);
                }
            }
            
        }

        // If RTC DS1388 ready, then copy GPS data to RTC all 5min
        if(millis() > starttime11 + 5*60*1000){
            starttime11 = millis();
            if(rtcOn == "DS1388" && RTC_ready == true && GPS_ready == true){
                api->getBoatDataValues(3,valueList);
                if(gpsdays->valid && gpsseconds->valid && hdop->valid){
                    long ts = tNMEA0183Msg::daysToTime_t(gpsdays->value - (30*365+7))+floor(gpsseconds->value); // Adjusted to reference year 2000 (-30 years and 7 days for switch years)
                    // sample input: date = "Dec 26 2009", time = "12:34:56"
                    // ds1388.adjust(DateTime("Dec 26 2009", "12:34:56"));
                    DateTime adjusttime(ts);
                    api->getLogger()->logDebug(GwLog::LOG,"Adjust RTC time: %04d/%02d/%02d %02d:%02d:%02d",adjusttime.year(), adjusttime.month(), adjusttime.day(), adjusttime.hour(), adjusttime.minute(), adjusttime.second());
                    // Adjust RTC time as unix time value
                    ds1388.adjust(adjusttime);
                }
            }
        }

        // Send 1Wire data for all temperature sensors all 2s
        if(millis() > starttime13 + 2000 && String(oneWireOn) == "DS18B20" && oneWire_ready == true){
            starttime13 = millis();
            float tempC;
            ds18b20.requestTemperatures();  // Collect all temperature values (max.8)
            for(int i=0;i<numberOfDevices; i++){
                // Send only one 1Wire data per loop step (time reduction)
                if(i == loopCounter % numberOfDevices){
                    if(ds18b20.getAddress(tempDeviceAddress, i)){
                        // Read temperature value in Celsius
                        tempC = ds18b20.getTempC(tempDeviceAddress); 
                    }
                    // Send to NMEA200 bus for each sensor with instance number
                    if(!isnan(tempC)){
                        sensors.onewireTemp[i] = tempC; // Save values in SensorData
                        api->getLogger()->logDebug(GwLog::DEBUG,"DS18B20-%1d Temp: %.1f",i,tempC);
                        SetN2kPGN130316(N2kMsg, 0, i, N2kts_OutsideTemperature, CToKelvin(tempC), N2kDoubleNA);
                        api->sendN2kMessage(N2kMsg);
                    }
                }    
            }
            loopCounter++;
        }

        // If GPS not ready or installed then send RTC time on bus all 500ms
        if(millis() > starttime12 + 500){
            starttime12 = millis();
            if((rtcOn == "DS1388" && RTC_ready == true && GPS_ready == false) || (rtcOn == "DS1388" && RTC_ready == true && GPS_ready == true && hdop->valid == false)){
                // Convert RTC time to Unix system time
                // https://de.wikipedia.org/wiki/Unixzeit
                const short daysOfYear[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
                long unixtime = ds1388.now().get();
                uint16_t year = ds1388.now().year();
                uint8_t month = ds1388.now().month();
                uint8_t hour = ds1388.now().hour();
                uint8_t minute = ds1388.now().minute();
                uint8_t second = ds1388.now().second();
                uint8_t day = ds1388.now().day();
                uint16_t switchYear = ((year-1)-1968)/4 - ((year-1)-1900)/100 + ((year-1)-1600)/400;
                long daysAt1970 = (year-1970)*365 + switchYear + daysOfYear[month-1] + day-1;
                // If switch year then add one day
                if ( (month>2) && (year%4==0 && (year%100!=0 || year%400==0)) ){
                    daysAt1970 += 1;
                }
                double sysTime = (hour * 3600) + (minute * 60) + second;
                if(!isnan(daysAt1970) && !isnan(sysTime)){
                    sensors.rtcYear = year; // Save values in SensorData
                    sensors.rtcMonth = month;
                    sensors.rtcDay = day;
                    sensors.rtcHour = hour;
                    sensors.rtcMinute = minute;
                    sensors.rtcSecond = second;
                    // api->getLogger()->logDebug(GwLog::LOG,"RTC time: %04d/%02d/%02d %02d:%02d:%02d",year, month, day, hour, minute, second);
                    // api->getLogger()->logDebug(GwLog::LOG,"Send PGN126992: %10d %10d",daysAt1970, (uint16_t)sysTime);
                    SetN2kPGN126992(N2kMsg,0,daysAt1970,sysTime,N2ktimes_LocalCrystalClock);
                    api->sendN2kMessage(N2kMsg);
                }
            }
        }

        // Send supply voltage value all 1s
        if(millis() > starttime5 + 1000 && String(powsensor1) == "off"){
            starttime5 = millis();
            #ifdef VOLTAGE_SENSOR
            sensors.batteryVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.53) * 2;   // Vin = 1/2 for OBP40
            #else
            sensors.batteryVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // Vin = 1/20 for OBP60    
            #endif
            sensors.batteryVoltage = sensors.batteryVoltage * vslope + voffset; // Calibration
            #ifdef LIPO_ACCU_1200
            if(sensors.batteryVoltage > 4.1){
                sensors.BatteryChargeStatus = 1;    // Charging active
            }
            else{
                sensors.BatteryChargeStatus = 0;    // Discharging
            }
            // Polynomfit for LiPo capacity calculation for 3,7V LiPo accus, 0...100%
            sensors.batteryLevelLiPo = sensors.batteryVoltage * sensors.batteryVoltage * 174.9513 + sensors.batteryVoltage * 1147,7686 + 1868.5120;
            // Limiter
            if(sensors.batteryLevelLiPo > 100){
                sensors.batteryLevelLiPo = 100;
            }
            if(sensors.batteryLevelLiPo < 0){
                sensors.batteryLevelLiPo = 0;
            }
            #endif
            // Save new data in average array
            batV.reading(int(sensors.batteryVoltage * 100));
            // Calculate the average values for different time lines from integer values
            sensors.batteryVoltage10 = batV.getAvg(10) / 100.0;
            sensors.batteryVoltage60 = batV.getAvg(60) / 100.0;
            sensors.batteryVoltage300 = batV.getAvg(300) / 100.0;
            // Send to NMEA200 bus
            if(!isnan(sensors.batteryVoltage)){
                SetN2kDCBatStatus(N2kMsg, 0, sensors.batteryVoltage, N2kDoubleNA, N2kDoubleNA, 1);
                api->sendN2kMessage(N2kMsg);
            }
        }

        // Send data from environment sensor all 2s
        if(millis() > starttime6 + 2000){
            starttime6 = millis();
            unsigned char TempSource = 2;       // Inside temperature
            unsigned char PressureSource = 0;   // Atmospheric pressure
            unsigned char HumiditySource = 0;   // Inside humidity
            if(envsensor == "BME280" && BME280_ready == true){
                sensors.airTemperature = bme280.readTemperature();
                sensors.airPressure = bme280.readPressure();
                sensors.airHumidity = bme280.readHumidity();
                // Send to NMEA200 bus
                if(!isnan(sensors.airTemperature)){
                    SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(sensors.airTemperature), N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
                if(!isnan(sensors.airHumidity)){
                    SetN2kPGN130313(N2kMsg, 0, 0,(tN2kHumiditySource) HumiditySource, sensors.airHumidity, N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
                if(!isnan(sensors.airPressure)){
                    SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) PressureSource, sensors.airPressure);
                    api->sendN2kMessage(N2kMsg);
                }
            }
            else if(envsensor == "BMP280" && BMP280_ready == true){
                sensors.airTemperature = bmp280.readTemperature();
                sensors.airPressure = bmp280.readPressure();
                // Send to NMEA200 bus
                if(!isnan(sensors.airTemperature)){
                    SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(sensors.airTemperature), N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
                if(!isnan(sensors.airPressure)){
                    SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) PressureSource, sensors.airPressure);
                    api->sendN2kMessage(N2kMsg);
                }
            }
            else if((envsensor == "BMP085" || envsensor == "BMP180") && BMP180_ready == true){
                sensors.airTemperature = bmp085.readTemperature();
                sensors.airPressure = bmp085.readPressure();
                // Send to NMEA200 bus
                if(!isnan(sensors.airTemperature)){
                    SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(sensors.airTemperature), N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
                if(!isnan(sensors.airPressure)){
                    SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) PressureSource, sensors.airPressure);
                    api->sendN2kMessage(N2kMsg);
                }
            }
            else if((envsensor == "SHT21" || envsensor == "HTU21") && SHT21_ready == true){
                sensors.airHumidity = sht21.readCompensatedHumidity();
                sensors.airTemperature = sht21.readTemperature();
                // Send to NMEA200 bus
                if(!isnan(sensors.airTemperature)){
                    SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(sensors.airTemperature), N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
                if(!isnan(sensors.airHumidity)){
                    SetN2kPGN130313(N2kMsg, 0, 0,(tN2kHumiditySource) HumiditySource, sensors.airHumidity, N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
            }                
        }

        // Send rotation angle all 500ms
        if(millis() > starttime7 + 500){
            starttime7 = millis();
            double rotationAngle=0;
            if(String(rotsensor) == "AS5600" && AS5600_ready == true && as5600.detectMagnet() == 1){
                rotationAngle = as5600.getRawAngle() * 0.087;       // 0...4095 segments = 0.087 degree
                // Offset correction
                if(rotoffset >= 0){
                    rotationAngle = rotationAngle + rotoffset;
                    rotationAngle = int(rotationAngle) % 360;
                }
                else{
                    rotationAngle = rotationAngle + 360 + rotoffset;
                    rotationAngle = int(rotationAngle) % 360;
                }
                // Send to NMEA200 bus as rudder angle values
                if(!isnan(rotationAngle) && String(rotfunction) == "Rudder"){
                    double rudder = rotationAngle - 180;            // Center position is 180°
                    // Rudder limits to +/-45°
                    if(rudder < -45){
                        rudder = -45;
                    }
                    if(rudder > 45){
                        rudder = 45;
                    }
                    SetN2kRudder(N2kMsg, DegToRad(rudder), 0, N2kRDO_NoDirectionOrder, PI);
                    api->sendN2kMessage(N2kMsg);
                }
                // Send to NMEA200 bus as wind angle values
                if(!isnan(rotationAngle) && String(rotfunction) == "Wind"){
                    SetN2kWindSpeed(N2kMsg, 1, 0, DegToRad(rotationAngle), N2kWind_Apprent);
                    api->sendN2kMessage(N2kMsg);
                }
                // Send to NMEA200 bus as trim angle values in [%]
                if(!isnan(rotationAngle) && (String(rotfunction) == "Mast" || String(rotfunction) == "Keel" || String(rotfunction) == "Trim" || String(rotfunction) == "Boom")){
                    int trim = rotationAngle * 100 / 360;           // 0...360° -> 0...100%
                    SetN2kTrimTab(N2kMsg, trim, trim);
                    api->sendN2kMessage(N2kMsg);
                }
                sensors.rotationAngle = DegToRad(rotationAngle);    // Data take over to page
                sensors.validRotAngle = true;                       // Valid true, magnet present
            }
            else{
                sensors.rotationAngle = 0;      // Center position 0°
                sensors.validRotAngle = false;  // Valid false, magnet missing
            }    
        }

        // Send battery power value all 1s
        if(millis() > starttime8 + 1000 && (String(powsensor1) == "INA219" || String(powsensor1) == "INA226")){
            starttime8 = millis();
            if(String(powsensor1) == "INA226" && INA226_1_ready == true){
                double voltage = ina226_1.getBusVoltage();
                // Limiter for voltage average building
                if(voltage < 0){
                    voltage = 0;
                }
                if(voltage > 30){
                    voltage = 30;
                }
                sensors.batteryVoltage = voltage;
                sensors.batteryCurrent = ina226_1.getCurrent() * corrFactor;
                // Eliminates bit jitter by zero current values
                float factor = maxCurrent / 100;
                if(sensors.batteryCurrent >= (-0.015 * factor) && sensors.batteryCurrent <= (0.015 * factor)){
                    sensors.batteryCurrent = 0;
                }
                // Save actual values in average arrays as integer values
                batV.reading(int(sensors.batteryVoltage * 100));
                batC.reading(int(sensors.batteryCurrent * 10));
                // Calculate the average values for different time lines from integer values
                sensors.batteryVoltage10 = batV.getAvg(10) / 100.0;
                sensors.batteryVoltage60 = batV.getAvg(60) / 100.0;
                sensors.batteryVoltage300 = batV.getAvg(300) / 100.0;
                sensors.batteryCurrent10 = batC.getAvg(10) / 10.0;
                sensors.batteryCurrent60 = batC.getAvg(60) / 10.0;
                sensors.batteryCurrent300 = batC.getAvg(300) / 10.0;
                sensors.batteryPower10 = sensors.batteryVoltage10 * sensors.batteryCurrent10;
                sensors.batteryPower60 = sensors.batteryVoltage60 * sensors.batteryCurrent60;
                sensors.batteryPower300 = sensors.batteryVoltage300 * sensors.batteryCurrent300;
//                sensors.batteryPower = ina226_1.getPower() * corrFactor;  // Real value
                sensors.batteryPower = sensors.batteryVoltage * sensors.batteryCurrent;  // Calculated power value (more stable)
            }
            // Send battery live data to NMEA200 bus
            if(!isnan(sensors.batteryVoltage) && !isnan(sensors.batteryCurrent)){
                SetN2kDCBatStatus(N2kMsg, 0, sensors.batteryVoltage, sensors.batteryCurrent, N2kDoubleNA, 1);
                api->sendN2kMessage(N2kMsg);
            }
        }

        // Send solar power value all 1s
        if(millis() > starttime9 + 1000 && (String(powsensor2) == "INA219" || String(powsensor2) == "INA226")){
            starttime9 = millis();
            if(String(powsensor2) == "INA226" && INA226_2_ready == true){
                double voltage = ina226_2.getBusVoltage();
                // Limiter for voltage average building
                if(voltage < 0){
                    voltage = 0;
                }
                if(voltage > 30){
                    voltage = 30;
                }
                sensors.solarVoltage = voltage;
                sensors.solarCurrent = ina226_2.getCurrent() * corrFactor;
                // Eliminates bit jitter by zero current values
                float factor = maxCurrent / 100;
                if(sensors.solarCurrent >= (-0.015 * factor) && sensors.solarCurrent <= (0.015 * factor)){
                    sensors.solarCurrent = 0;
                }
                
                // Calculate power value
                sensors.solarPower = sensors.solarVoltage * sensors.solarCurrent; // more stable
            }
            // Send solar live data to NMEA200 bus
            if(!isnan(sensors.solarVoltage) && !isnan(sensors.solarCurrent)){
                SetN2kDCBatStatus(N2kMsg, 1, sensors.solarVoltage, sensors.solarCurrent, N2kDoubleNA, 1);
                api->sendN2kMessage(N2kMsg);
            }
        }

        // Send generator power value all 1s
        if(millis() > starttime10 + 1000 && (String(powsensor3) == "INA219" || String(powsensor3) == "INA226")){
            starttime10 = millis();
            if(String(powsensor3) == "INA226" && INA226_3_ready == true){
                double voltage = ina226_3.getBusVoltage();
                // Limiter for voltage average building
                if(voltage < 0){
                    voltage = 0;
                }
                if(voltage > 30){
                    voltage = 30;
                }
                sensors.generatorVoltage = voltage;
                sensors.generatorCurrent = ina226_3.getCurrent() * corrFactor;
                // Eliminates bit jitter by zero current values
                float factor = maxCurrent / 100;
                if(sensors.generatorCurrent >= (-0.015 * factor) && sensors.generatorCurrent <= (0.015 * factor)){
                    sensors.generatorCurrent = 0;
                }
                
                // Calculate power value
                sensors.generatorPower = sensors.generatorVoltage * sensors.generatorCurrent; // more stable
            }
            // Send solar live data to NMEA200 bus
            if(!isnan(sensors.generatorVoltage) && !isnan(sensors.generatorCurrent)){
                SetN2kDCBatStatus(N2kMsg, 2, sensors.generatorVoltage, sensors.generatorCurrent, N2kDoubleNA, 1);
                api->sendN2kMessage(N2kMsg);
            }
        }

        shared->setSensorData(sensors);
    }
    vTaskDelete(NULL);
}


void createSensorTask(SharedData *shared){
    xTaskCreate(sensorTask,"readSensors",10000,shared,3,NULL);
}
#endif
