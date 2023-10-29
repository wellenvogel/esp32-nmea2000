#include "GwIicTask.h"
#include "GwHardware.h"
#ifdef _GWIIC
    #include <Wire.h>
#endif
#ifdef GWSHT3X
    #include "SHT3X.h"
#endif
#ifdef GWQMP6988
    #include "QMP6988.h"
#endif
#include "GwTimer.h"
#include "N2kMessages.h"
#include "GwHardware.h"
#include "GwXdrTypeMappings.h"
#ifdef GWBME280
    #include <Adafruit_BME280.h>
#endif
//#define GWSHT3X -1

#ifndef GWIIC_SDA
    #define GWIIC_SDA -1
#endif
#ifndef GWIIC_SCL
    #define GWIIC_SCL -1
#endif
class SHT3XConfig{
    public:
    String tempTransducer;
    String humidTransducer;
    int iid;
    bool tempActive;
    bool humidActive;
    long interval;
    tN2kHumiditySource humiditySource;
    tN2kTempSource tempSource;
    SHT3XConfig(GwConfigHandler *config){
        tempTransducer=config->getString(GwConfigDefinitions::SHT3XTempName);
        humidTransducer=config->getString(GwConfigDefinitions::SHT3XHumidName);
        iid=config->getInt(GwConfigDefinitions::SHT3Xiid,99);
        tempActive=config->getBool(GwConfigDefinitions::iicSHT3XTemp);
        humidActive=config->getBool(GwConfigDefinitions::iicSHT3XHumid);
        interval=config->getInt(GwConfigDefinitions::SHT3Xinterval);
        interval*=1000;
        humiditySource=(tN2kHumiditySource)(config->getInt(GwConfigDefinitions::SHT3XHumSource));
        tempSource=(tN2kTempSource)(config->getInt(GwConfigDefinitions::SHT3XTempSource));
    }
};

class QMP6988Config{
    public:
        String transducer="Pressure";
        int iid=99;
        bool active=true;
        long interval=2000;
        tN2kPressureSource source=tN2kPressureSource::N2kps_Atmospheric;
        float offset=0;
        QMP6988Config(GwConfigHandler *config){
            transducer=config->getString(GwConfigDefinitions::QMP6988PName);
            iid=config->getInt(GwConfigDefinitions::QMP6988iid);
            active=config->getBool(GwConfigDefinitions::QMP6988act);
            interval=config->getInt(GwConfigDefinitions::QMP6988interval);
            interval*=1000;
            offset=config->getInt(GwConfigDefinitions::QMP6988POffset);
        }
};

class BME280Config{
    public:
    bool pressureActive=true;
    bool tempActive=true;
    bool humidActive=true;
    tN2kTempSource tempSource=tN2kTempSource::N2kts_InsideTemperature;
    tN2kHumiditySource humidSource=tN2kHumiditySource::N2khs_InsideHumidity;
    tN2kPressureSource pressureSource=tN2kPressureSource::N2kps_Atmospheric;
    int iid=99;
    long interval=2000;
    String tempXdrName="Temperature";
    String humidXdrName="Humidity";
    String pressXdrName="Pressure";
    float tempOffset=0;
    float pressureOffset=0;
    BME280Config(GwConfigHandler *config){
        pressureActive=config->getBool(GwConfigDefinitions::iicBME280Press);
        tempActive=config->getBool(GwConfigDefinitions::iicBME280Temp);
        humidActive=config->getBool(GwConfigDefinitions::iicBME280Humid);
        tempSource=(tN2kTempSource)config->getInt(GwConfigDefinitions::BME280TSource);
        humidSource=(tN2kHumiditySource)config->getInt(GwConfigDefinitions::BME280HumSource);
        iid=config->getInt(GwConfigDefinitions::BME280iid);
        interval=1000*config->getInt(GwConfigDefinitions::BME280interval);
        tempXdrName=config->getString(GwConfigDefinitions::BME280TempName);
        humidXdrName=config->getString(GwConfigDefinitions::BME280HumidName);
        pressXdrName=config->getString(GwConfigDefinitions::BME280PressName);
        tempOffset=config->getInt(GwConfigDefinitions::BME280TOffset);
        pressureOffset=config->getInt(GwConfigDefinitions::BME280POffset);
    }
};
void runIicTask(GwApi *api);

void initIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        vTaskDelete(NULL);
        return;
    #else
    bool addTask=false;
    #ifdef GWSHT3X
        api->addCapability("SHT3X","true");
        LOG_DEBUG(GwLog::LOG,"SHT3X configured");
        SHT3XConfig sht3xConfig(api->getConfig());
        if (sht3xConfig.humidActive && ! sht3xConfig.humidTransducer.isEmpty()){
            LOG_DEBUG(GwLog::DEBUG,"SHT3X humidity measure active, adding capability and xdr mappings");
            //add XDR mapping for humidity
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRHUMIDITY;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_HUMIDITY_ACTUALHUMIDITY;
            xdr.selector=(int)sht3xConfig.humiditySource;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=sht3xConfig.iid;
            xdr.xdrName=sht3xConfig.humidTransducer;
            api->addXdrMapping(xdr);
        }
        if (sht3xConfig.tempActive && ! sht3xConfig.tempTransducer.isEmpty()){
            LOG_DEBUG(GwLog::DEBUG,"SHT3X temperature measure active, adding capability and xdr mappings");
            //add XDR mapping for humidity
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRTEMP;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_TEMPERATURE_ACTUALTEMPERATURE;
            xdr.selector=(int)sht3xConfig.tempSource;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=sht3xConfig.iid;
            xdr.xdrName=sht3xConfig.tempTransducer;
            api->addXdrMapping(xdr);
        }
        if (sht3xConfig.tempActive || sht3xConfig.humidActive) addTask=true;
    #endif
    #ifdef GWQMP6988
        api->addCapability("QMP6988","true");
        QMP6988Config qmp6988Config(api->getConfig());
        if (qmp6988Config.active) {
            LOG_DEBUG(GwLog::LOG,"QMP6988 configured, adding capability and xdr mappings");
            addTask=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRPRESSURE;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.selector=(int)qmp6988Config.source;
            xdr.instanceId=qmp6988Config.iid;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.xdrName=qmp6988Config.transducer;
            api->addXdrMapping(xdr);
        }
        else{
            LOG_DEBUG(GwLog::LOG,"QMP6988 configured but disabled");
        }
    #endif
    #ifdef GWBME280
        api->addCapability("BME280","true");
        BME280Config bme280Config(api->getConfig());
        bool bme280Active=false;
        if (bme280Config.pressureActive){
            LOG_DEBUG(GwLog::DEBUG,"BME280 pressure active, adding capability and xdr mapping");
            bme280Active=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRPRESSURE;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.selector=(int)bme280Config.pressureSource;
            xdr.instanceId=bme280Config.iid;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.xdrName=bme280Config.pressXdrName;
            api->addXdrMapping(xdr);
        }
        if (bme280Config.tempActive){
            LOG_DEBUG(GwLog::DEBUG,"BME280 temperature active, adding capability and xdr mapping");
            bme280Active=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRTEMP;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_TEMPERATURE_ACTUALTEMPERATURE;
            xdr.selector=(int)bme280Config.tempSource;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=bme280Config.iid;
            xdr.xdrName=bme280Config.tempXdrName;
            api->addXdrMapping(xdr);
        }
        if (bme280Config.humidActive){
            LOG_DEBUG(GwLog::DEBUG,"BME280 humidity active, adding capability and xdr mapping");
            bme280Active=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRHUMIDITY;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_HUMIDITY_ACTUALHUMIDITY;
            xdr.selector=(int)bme280Config.humidSource;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=bme280Config.iid;
            xdr.xdrName=bme280Config.humidXdrName;
            api->addXdrMapping(xdr);
        }
        if (! bme280Active){
            LOG_DEBUG(GwLog::DEBUG,"BME280 configured but disabled");
        }
        else{
            addTask=true;
        }
    #endif
    if (addTask){
        api->addUserTask(runIicTask,"iicTask",3000);
    }
}
void runIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC 
        LOG_DEBUG(GwLog::LOG,"no iic defined, iic task stopped");
        vTaskDelete(NULL);
        return;
    #endif
    LOG_DEBUG(GwLog::LOG,"iic task started");
    bool rt=Wire.begin(GWIIC_SDA,GWIIC_SCL);
    if (! rt){
        LOG_DEBUG(GwLog::ERROR,"unable to initialize IIC");
        vTaskDelete(NULL);
        return;
    }
    GwConfigHandler *config=api->getConfig();
    bool runLoop=false;
    GwIntervalRunner timers;
    int counterId=api->addCounter("iicsensors");
    #ifdef GWSHT3X
        SHT3X *sht3x=nullptr;
        int addr=GWSHT3X;
        if (addr < 0) addr=0x44; //default
        SHT3XConfig sht3xConfig(config);
        if (sht3xConfig.humidActive || sht3xConfig.tempActive){
            sht3x=new SHT3X();
            sht3x->init(addr,&Wire);
            LOG_DEBUG(GwLog::LOG,"initialized SHT3X at address %d, interval %ld",(int)addr,sht3xConfig.interval);
            runLoop=true;
            timers.addAction(sht3xConfig.interval,[logger,api,sht3x,sht3xConfig,counterId](){
                int rt=0;
                if ((rt=sht3x->get())==0){
                    double temp=sht3x->cTemp;
                    temp=CToKelvin(temp);
                    double humid=sht3x->humidity;
                    LOG_DEBUG(GwLog::DEBUG,"SHT3X measure temp=%2.1f, humid=%2.0f",(float)temp,(float)humid);
                    tN2kMsg msg;
                    if (sht3xConfig.humidActive){
                        SetN2kHumidity(msg,1,sht3xConfig.iid,sht3xConfig.humiditySource,humid);
                        api->sendN2kMessage(msg);
                        api->increment(counterId,"SHT3Xhum");
                    }
                    if (sht3xConfig.tempActive){
                        SetN2kTemperature(msg,1,sht3xConfig.iid,sht3xConfig.tempSource,temp);
                        api->sendN2kMessage(msg);
                        api->increment(counterId,"SHT3Xtemp");
                    }
                }
                else{
                    LOG_DEBUG(GwLog::DEBUG,"unable to query SHT3X: %d",rt);
                }    
            });
        }
    #endif
    #ifdef GWQMP6988
        int qaddr=GWQMP6988;
        if (qaddr < 0) qaddr=0x56;
        QMP6988Config qmp6988Config(api->getConfig());
        QMP6988 *qmp6988=nullptr;
        if (qmp6988Config.active){
            runLoop=true;
            qmp6988=new QMP6988();
            qmp6988->init(qaddr,&Wire);
            LOG_DEBUG(GwLog::LOG,"initialized QMP6988 at address %d, interval %ld",qaddr,qmp6988Config.interval);
            timers.addAction(qmp6988Config.interval,[logger,api,qmp6988,qmp6988Config,counterId](){
                float pressure=qmp6988->calcPressure();
                float computed=pressure+qmp6988Config.offset;
                LOG_DEBUG(GwLog::DEBUG,"qmp6988 measure %2.0fPa, computed %2.0fPa",pressure,computed);
                tN2kMsg msg;
                SetN2kPressure(msg,1,qmp6988Config.iid,tN2kPressureSource::N2kps_Atmospheric,computed);
                api->sendN2kMessage(msg);
                api->increment(counterId,"QMP6988press");
            });
        }
    #endif
    #ifdef GWBME280
        int baddr=GWBME280;
        if (baddr < 0) baddr=0x76;
        BME280Config bme280Config(api->getConfig());
        if (bme280Config.tempActive || bme280Config.pressureActive|| bme280Config.humidActive){
            Adafruit_BME280 *bme280=new Adafruit_BME280();
            if (bme280->begin(baddr,&Wire)){
                uint32_t sensorId=bme280->sensorID();
                bool hasHumidity=sensorId == 0x60; //BME280, else BMP280
                if (bme280Config.tempOffset != 0){
                    bme280->setTemperatureCompensation(bme280Config.tempOffset);
                }
                if (hasHumidity || bme280Config.tempActive || bme280Config.pressureActive)
                {
                    LOG_DEBUG(GwLog::LOG, "initialized BME280 at %d, sensorId 0x%x", baddr, sensorId);
                    timers.addAction(bme280Config.interval, [logger, api, bme280, bme280Config, counterId, hasHumidity](){
                        if (bme280Config.pressureActive){
                            float pressure=bme280->readPressure();
                            float computed=pressure+bme280Config.pressureOffset;
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure %2.0fPa, computed %2.0fPa",pressure,computed);
                            tN2kMsg msg;
                            SetN2kPressure(msg,1,bme280Config.iid,bme280Config.pressureSource,computed);
                            api->sendN2kMessage(msg);
                            api->increment(counterId,"BME280press");
                        }
                        if (bme280Config.tempActive){
                            float temperature=bme280->readTemperature(); //offset is handled internally
                            temperature=CToKelvin(temperature);
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure temp=%2.1f",temperature);
                            tN2kMsg msg;
                            SetN2kTemperature(msg,1,bme280Config.iid,bme280Config.tempSource,temperature);
                            api->sendN2kMessage(msg);
                            api->increment(counterId,"BME280temp");
                        }
                        if (bme280Config.humidActive && hasHumidity){
                            float humidity=bme280->readHumidity();
                            LOG_DEBUG(GwLog::DEBUG,"BME280 read humid=%02.0f",humidity);
                            tN2kMsg msg; 
                            SetN2kHumidity(msg,1,bme280Config.iid,bme280Config.humidSource,humidity);
                            api->sendN2kMessage(msg);
                            api->increment(counterId,"BME280hum");
                        }
                    });
                    runLoop = true;
                }
                else{
                    LOG_DEBUG(GwLog::ERROR,"BME280 only humidity active, but sensor does not have it");
                }
            }
            else{
                LOG_DEBUG(GwLog::ERROR,"unable to initialize BME280 sensor at address %d",baddr);
            }
        }
    #endif
    if (! runLoop){
        LOG_DEBUG(GwLog::LOG,"nothing to do for IIC task, finish");
        vTaskDelete(NULL);
        return;
    }
    while(true){
        delay(100);
        timers.loop();
    }
    vTaskDelete(NULL);
    #endif
}
