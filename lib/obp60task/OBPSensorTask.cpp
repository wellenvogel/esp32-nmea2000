#ifdef BOARD_NODEMCU32S_OBP60
#include <Adafruit_Sensor.h>            // Adafruit Lib for sensors
#include <Adafruit_BME280.h>            // Adafruit Lib for BME280
#include <Adafruit_BMP280.h>            // Adafruit Lib for BMP280
#include <Adafruit_BMP085.h>            // Adafruit Lib for BMP085 and BMP180
#include <HTU21D.h>                     // Lib for SHT21/HTU21
#include <AS5600.h>                     // Lib for magnetic rotation sensor AS5600
#include "INA226.h"                     // Lib for power management IC INA226
#include <Ticker.h>                     // Timer Lib for timer interrupts       
#include "OBPSensorTask.h"
#include "OBP60Hardware.h"
#include "N2kMessages.h"
#include "NMEA0183.h"
#include "ObpNmea0183.h"
#include "OBP60ExtensionPort.h"  

// Timer Interrupts for hardware functions
void underVoltageDetection();
Ticker Timer1(underVoltageDetection, 1);    // Start Timer1 with maximum speed with 1ms
Ticker Timer2(blinkingFlashLED, 500);       // Satrt Timer2 for flash LED all 500ms

// Undervoltage function for shutdown display
void underVoltageDetection(){
    float actVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // V = 1/20 * Vin
    long starttime = 0;
    static bool undervoltage = false;

    if(actVoltage < MIN_VOLTAGE){
        if(undervoltage == false){
            starttime = millis();
            undervoltage = true;
        }
        if(millis() > starttime + POWER_FAIL_TIME){
            Timer1.stop();                          // Stop Timer1
            setPortPin(OBP_BACKLIGHT_LED, false);   // Backlight Off
            setPortPin(OBP_FLASH_LED, false);       // Flash LED Off
            setPortPin(OBP_POWER_33, false);        // Power rail 3.3V Off
            buzzer(TONE4, 20);                      // Buzzer tone 4kHz 20ms
            setPortPin(OBP_POWER_50, false);        // Power rail 5.0V Off
            // Shutdown EInk display
            display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
            display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false); // Partial update
            display.update();
            // Stop system
            while(true){
                esp_deep_sleep_start();             // Deep Sleep without weakup. Weakup only after power cycle (restart).
            }
        }
    }
    else{
        undervoltage = false;
    }
}

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
    INA226 ina226_1(INA226_I2C_ADDR1);// Power management sensor INA226

    // Init sensor stuff
    bool gps_ready = false;         // GPS initialized and ready to use
    bool BME280_ready = false;      // BME280 initialized and ready to use
    bool BMP280_ready = false;      // BMP280 initialized and ready to use
    bool BMP180_ready = false;      // BMP180 initialized and ready to use
    bool SHT21_ready = false;       // SHT21 initialized and ready to use
    bool AS5600_ready = false;      // AS5600 initialized and ready to use
    bool INA226_1_ready = false;    // INA226_1 initialized and ready to use

    // Start timer interrupts
    bool uvoltage = api->getConfig()->getConfigItem(api->getConfig()->underVoltage,true)->asBoolean();
    if(uvoltage == true){
        Timer1.start();     // Start Timer1 for undervoltage detection
    }
    Timer2.start();         // Start Timer2 for blinking LED

    // Settings for NMEA0183
    String nmea0183Mode = api->getConfig()->getConfigItem(api->getConfig()->serialDirection, true)->asString();
    api->getLogger()->logDebug(GwLog::LOG, "NMEA0183 Mode is: %s", nmea0183Mode);
    pinMode(OBP_DIRECTION_PIN, OUTPUT);
    if (String(nmea0183Mode) == "receive" || String(nmea0183Mode) == "off")
    {
        digitalWrite(OBP_DIRECTION_PIN, false);
    }
    if (String(nmea0183Mode) == "send")
    {
        digitalWrite(OBP_DIRECTION_PIN, true);
    }

    // Setting for GPS sensors
    String gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asString();
        if(String(gpsOn) == "NEO-6M"){
            Serial2.begin(9600, SERIAL_8N1, OBP_GPS_TX, -1); // GPS RX unused in hardware (-1)
            if (!Serial2) {     
                api->getLogger()->logDebug(GwLog::ERROR,"GPS modul NEO-6M not found, check wiring");
                gps_ready = false;
                }
            else{
                api->getLogger()->logDebug(GwLog::LOG,"GPS modul NEO-M6 found");
                NMEA0183.SetMessageStream(&Serial2);
                NMEA0183.Open();
                gps_ready = true;
            }
        }
        if(String(gpsOn) == "NEO-M8N"){
            Serial2.begin(9600, SERIAL_8N1, OBP_GPS_TX, -1); // GPS RX unused in hardware (-1)
            if (!Serial2) {     
                api->getLogger()->logDebug(GwLog::ERROR,"GPS modul NEO-M8N not found, check wiring");
                gps_ready = false;
                }
            else{
                api->getLogger()->logDebug(GwLog::LOG,"GPS modul NEO-M8N found");
                NMEA0183.SetMessageStream(&Serial2);
                NMEA0183.Open();
                gps_ready = true;
            }
        }

    // Settings for temperature sensors
    String tempSensor = api->getConfig()->getConfigItem(api->getConfig()->useTempSensor,true)->asString();
    if(String(tempSensor) == "DS18B20"){
        api->getLogger()->logDebug(GwLog::LOG,"1Wire Mode is On");
    }
    else{
        api->getLogger()->logDebug(GwLog::LOG,"1Wire Mode is Off");
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
            sensors.airPressure = bme280.readPressure()/100;
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
            sensors.airPressure  =bmp280.readPressure()/100;
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
            sensors.airPressure  =bmp085.readPressure()/100;
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

    // Settings for power amangement sensors INA226 #1 on I2C bus
    String powsensor1 = api->getConfig()->getConfigItem(api->getConfig()->usePowSensor1, true)->asString();
    String shunt1 = api->getConfig()->getConfigItem(api->getConfig()->shunt1, true)->asString();

    float shuntResistor = 1.0;  // Default value for shunt resistor
    float current = 10.0;       // Default value for max. current
    float corrFactor = 1;       // Correction factor for fix calibration
    
    if(String(powsensor1) == "INA226"){
        if (!ina226_1.begin()){
            api->getLogger()->logDebug(GwLog::ERROR,"Modul 1 INA226 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::LOG,"Modul 1 INA226 found");
            shuntResistor = SHUNT_VOLTAGE / float(shunt1.toInt());  // Calculate shunt resisitor for max. shunt voltage 75mV
            current = float(shunt1.toInt());
            api->getLogger()->logDebug(GwLog::LOG,"Calibation INA226, Imax:%3.0fA Rs:%7.5fOhm Us:%5.3f", current, shuntResistor, SHUNT_VOLTAGE);
//            ina226_1.setMaxCurrentShunt(current, shuntResistor);
            ina226_1.setMaxCurrentShunt(10, 0.01);  // Calibration with fix values (because the original values outer range)
            corrFactor = (current / 10) * (0.001 / shuntResistor) / (current / 100); // correction factor for fix calibration
            sensors.batteryVoltage = ina226_1.getBusVoltage();
            sensors.batteryCurrent = ina226_1.getCurrent() * corrFactor;
            sensors.batteryPower = ina226_1.getPower() * corrFactor;
            INA226_1_ready = true;
        }
    }

    int rotoffset = api->getConfig()->getConfigItem(api->getConfig()->rotOffset,true)->asInt();
    long starttime0 = millis();     // GPS update all 1s
    long starttime5 = millis();     // Voltage update all 1s
    long starttime6 = millis();     // Environment sensor update all 1s
    long starttime7 = millis();     // Rotation sensor update all 500ms
    long starttime8 = millis();     // Power management sensor update all 1s

    tN2kMsg N2kMsg;
    shared->setSensorData(sensors); //set initially read values

    // Sensor task loop runs with 100ms
    //####################################################################################

    while (true){
        delay(100);                 // Loop time 100ms
        Timer1.update();            // Update for Timer1
        Timer2.update();            // Update for Timer2
        if (millis() > starttime0 + 1000)
        {
            starttime0 = millis();
            // Send NMEA0183 GPS data on several bus systems all 1000ms
            if (gps_ready == true)
            {
                SNMEA0183Msg NMEA0183Msg;
                while (NMEA0183.GetMessageCor(NMEA0183Msg))
                {
                    api->sendNMEA0183Message(NMEA0183Msg);
                }
            }
            
        }
        // Read sensors and set values in sensor data
        // Send supplay voltage value all 1s
        if(millis() > starttime5 + 1000 && String(powsensor1) == "off"){
            starttime5 = millis();
            sensors.batteryVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // Vin = 1/20
            // Send to NMEA200 bus
            if(!isnan(sensors.batteryVoltage)){
                SetN2kDCBatStatus(N2kMsg, 0, sensors.batteryVoltage, N2kDoubleNA, N2kDoubleNA, 1);
                api->sendN2kMessage(N2kMsg);
            }
        }

        // Send data from environment sensor all 1s
        if(millis() > starttime6 + 2000){
            starttime6 = millis();
            unsigned char TempSource = 2;       // Inside temperature
            unsigned char PressureSource = 0;   // Atmospheric pressure
            unsigned char HumiditySource=0;     // Inside humidity
            if(envsensor == "BME280" && BME280_ready == true){
                sensors.airTemperature = bme280.readTemperature();
                sensors.airPressure = bme280.readPressure()/100;
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
                    SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) mBarToPascal(PressureSource), sensors.airPressure);
                    api->sendN2kMessage(N2kMsg);
                }
            }
            else if(envsensor == "BMP280" && BMP280_ready == true){
                sensors.airTemperature = bmp280.readTemperature();
                sensors.airPressure  =bmp280.readPressure()/100;
                // Send to NMEA200 bus
                if(!isnan(sensors.airTemperature)){
                    SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(sensors.airTemperature), N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
                if(!isnan(sensors.airPressure)){
                    SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) mBarToPascal(PressureSource), sensors.airPressure);
                    api->sendN2kMessage(N2kMsg);
                }
            }
            else if((envsensor == "BMP085" || envsensor == "BMP180") && BMP180_ready == true){
                sensors.airTemperature = bmp085.readTemperature();
                sensors.airPressure  =bmp085.readPressure()/100;
                // Send to NMEA200 bus
                if(!isnan(sensors.airTemperature)){
                    SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(sensors.airTemperature), N2kDoubleNA);
                    api->sendN2kMessage(N2kMsg);
                }
                if(!isnan(sensors.airPressure)){
                    SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) mBarToPascal(PressureSource), sensors.airPressure);
                    api->sendN2kMessage(N2kMsg);
                }
            }
            else if((envsensor == "SHT21" || envsensor == "HTU21") && SHT21_ready == true){
                sensors.airHumidity = sht21.readCompensatedHumidity();
                sensors.airHumidity = sht21.readTemperature();
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

        // Send power management value all 1s
        if(millis() > starttime8 + 1000 && (String(powsensor1) == "INA219" || String(powsensor1) == "INA226")){
            starttime8 = millis();
            if(String(powsensor1) == "INA226" && INA226_1_ready == true){
                sensors.batteryVoltage = ina226_1.getBusVoltage();
                sensors.batteryCurrent = ina226_1.getCurrent() * corrFactor;
                sensors.batteryPower = ina226_1.getPower() * corrFactor;
            }
            // Send battery data to NMEA200 bus
            if(!isnan(sensors.batteryVoltage) && !isnan(sensors.batteryCurrent)){
                SetN2kDCBatStatus(N2kMsg, 0, sensors.batteryVoltage, sensors.batteryCurrent, N2kDoubleNA, 1);
//                SetN2kDCBatStatus(N2kMsg, 0, sensors.batteryVoltage, sensors.batteryCurrent, sensors.batteryPower, 1);
                api->sendN2kMessage(N2kMsg);
            }
        }

        shared->setSensorData(sensors);
    }
    vTaskDelete(NULL);
}


void createSensorTask(SharedData *shared){
    xTaskCreate(sensorTask,"readSensors",4000,shared,3,NULL);
}
#endif