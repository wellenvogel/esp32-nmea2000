#ifdef HAS_BME280
#include "GwBME280Task.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <N2kTypes.h>
#include <N2kMessages.h>

#include "movingAvg.h"                  // Lib for moving average building

void BME280Task(void *param){
    GwApi *api=(GwApi*)param;
    GwLog *logger=api->getLogger();
    tN2kMsg N2kMsg;
    TwoWire I2CBME = TwoWire(0);
    Adafruit_BME280 bme;

    signed int PressureCalib=api->getConfig()->getInt(api->getConfig()->PressureCalib);
    unsigned char TempSource=api->getConfig()->getInt(api->getConfig()->Temperature);
    unsigned char PressureSource=api->getConfig()->getInt(api->getConfig()->Pressure);
    unsigned char HumiditySource=api->getConfig()->getInt(api->getConfig()->Humidity);

    unsigned long timerPressHistory = millis(); // Update pressure history every hour
    double Temperature;
    double Humidity;
    const int avgsizePress = 60;                // Size of moving average array
    constexpr int arrayPressure{avgsizePress};
    movingAvg Pavg(arrayPressure);              // Moving average of pressure readings (10*Pa)
    Pavg.begin();
    int pressureHistory[12] = {0};              // Pressure readings every hour (in 10*Pa)
    int idxPressHist=0;

    bool OK_BME280 = false;                     // BME280 initialized

    // Init
    I2CBME.begin(BME_SDA,BME_SCL,100000);

    if (!bme.begin(0x77,&I2CBME)) {
        if (!bme.begin(0x76,&I2CBME)) {
            LOG_DEBUG(GwLog::DEBUG,"BME280 not found. Check wiring");
            vTaskDelete(NULL);
            return;
        }
        else {
            for (int i=1; i<=avgsizePress+1; ++i) {
                Pavg.reading((int)(bme.readPressure()/10)); // Initialize avg array of pressure sensor (in 10*Pa)
            }
            OK_BME280 = true;
        }
    }
    while(true) {
        delay(1000);
        // Senf environment data every second (lower rate produce blank value on some NMEA2000 repeaters)
        if (OK_BME280) {
            Pavg.reading((int)(bme.readPressure()/10));
            Temperature=bme.readTemperature();
            Humidity=bme.readHumidity();
            SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(Temperature), N2kDoubleNA);
            api->sendN2kMessage(N2kMsg,true);
            SetN2kPGN130313(N2kMsg, 0, 0,(tN2kHumiditySource) HumiditySource, Humidity, N2kDoubleNA);
            api->sendN2kMessage(N2kMsg,true);
            SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) PressureSource, mBarToPascal((Pavg.getAvg(60)*1.0)/10+PressureCalib/100)); // Presure avg on 1min
            api->sendN2kMessage(N2kMsg,true);
        }

        // *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT 
        // Update pressure history every hour
        if ((millis()-timerPressHistory)>3600000) {
            timerPressHistory = millis();
            if (OK_BME280) {
                if (idxPressHist==10) {
                    for (int i = 0; i < 9; i++) {
                        pressureHistory[i] = pressureHistory[i + 1];
                    }
                    pressureHistory[idxPressHist - 1] = Pavg.getAvg(60);
                }
                else {
                    pressureHistory[idxPressHist] = Pavg.getAvg(60);
                    idxPressHist++;
                }
            }
        }
        // *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT *** UNDER DEVELOPMENT 

    }
    vTaskDelete(NULL);    
}
#endif
