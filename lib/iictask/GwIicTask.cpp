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

#define CFG_GET(cfg,name,prefix) \
    cfg->getValue(name, GwConfigDefinitions::prefix ## name)

#define CSHT3X(name) \
    CFG_GET(config,name,SHT3X)
#define CQMP6988(name) \
    CFG_GET(config,name,QMP6988)
#define CBME280(name) \
    CFG_GET(config,name,BME280)    
class SHT3XConfig{
    public:
    String tmNam;
    String huNam;
    int iid;
    bool tmAct;
    bool huAct;
    long intv;
    tN2kHumiditySource huSrc;
    tN2kTempSource tmSrc;
    SHT3XConfig(GwConfigHandler *config){
        CSHT3X(tmNam);
        CSHT3X(huNam);
        CSHT3X(iid);
        CSHT3X(tmAct);
        CSHT3X(huAct);
        CSHT3X(intv);
        intv*=1000;
        CSHT3X(huSrc);
        CSHT3X(tmSrc);
    }
};

class QMP6988Config{
    public:
        String prNam="Pressure";
        int iid=99;
        bool prAct=true;
        long intv=2000;
        tN2kPressureSource source=tN2kPressureSource::N2kps_Atmospheric;
        float prOff=0;
        QMP6988Config(GwConfigHandler *config){
            CQMP6988(prNam);
            CQMP6988(iid);
            CQMP6988(prAct);
            CQMP6988(intv);
            intv*=1000;
            CQMP6988(prOff);
        }
};

class BME280Config{
    public:
    bool prAct=true;
    bool tmAct=true;
    bool huAct=true;
    tN2kTempSource tmSrc=tN2kTempSource::N2kts_InsideTemperature;
    tN2kHumiditySource huSrc=tN2kHumiditySource::N2khs_InsideHumidity;
    tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
    int iid=99;
    long intv=2000;
    String tmNam="Temperature";
    String huNam="Humidity";
    String prNam="Pressure";
    float tmOff=0;
    float prOff=0;
    BME280Config(GwConfigHandler *config){
        CBME280(prAct);
        CBME280(tmAct);
        CBME280(huAct);
        CBME280(tmSrc);
        CBME280(huSrc);
        CBME280(iid);
        CBME280(intv);
        intv*=1000;
        CBME280(tmNam);
        CBME280(huNam);
        CBME280(prNam);
        CBME280(tmOff);
        CBME280(prOff);
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
        if (sht3xConfig.huAct && ! sht3xConfig.humidTransducer.isEmpty()){
            LOG_DEBUG(GwLog::DEBUG,"SHT3X humidity measure active, adding capability and xdr mappings");
            //add XDR mapping for humidity
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRHUMIDITY;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_HUMIDITY_ACTUALHUMIDITY;
            xdr.selector=(int)sht3xConfig.huSrc;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=sht3xConfig.iid;
            xdr.xdrName=sht3xConfig.humidTransducer;
            api->addXdrMapping(xdr);
        }
        if (sht3xConfig.tmAct && ! sht3xConfig.tName.isEmpty()){
            LOG_DEBUG(GwLog::DEBUG,"SHT3X temperature measure active, adding capability and xdr mappings");
            //add XDR mapping for humidity
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRTEMP;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_TEMPERATURE_ACTUALTEMPERATURE;
            xdr.selector=(int)sht3xConfig.tmSrc;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=sht3xConfig.iid;
            xdr.xdrName=sht3xConfig.tName;
            api->addXdrMapping(xdr);
        }
        if (sht3xConfig.tmAct || sht3xConfig.huAct) addTask=true;
    #endif
    #ifdef GWQMP6988
        api->addCapability("QMP6988","true");
        QMP6988Config qmp6988Config(api->getConfig());
        if (qmp6988Config.prAct) {
            LOG_DEBUG(GwLog::LOG,"QMP6988 configured, adding capability and xdr mappings");
            addTask=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRPRESSURE;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.selector=(int)qmp6988Config.source;
            xdr.instanceId=qmp6988Config.iid;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.xdrName=qmp6988Config.prName;
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
        if (bme280Config.prAct){
            LOG_DEBUG(GwLog::DEBUG,"BME280 pressure active, adding capability and xdr mapping");
            bme280Active=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRPRESSURE;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.selector=(int)bme280Config.prSrc;
            xdr.instanceId=bme280Config.iid;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.xdrName=bme280Config.prNam;
            api->addXdrMapping(xdr);
        }
        if (bme280Config.tmAct){
            LOG_DEBUG(GwLog::DEBUG,"BME280 temperature active, adding capability and xdr mapping");
            bme280Active=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRTEMP;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_TEMPERATURE_ACTUALTEMPERATURE;
            xdr.selector=(int)bme280Config.tmSrc;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=bme280Config.iid;
            xdr.xdrName=bme280Config.tmNam;
            api->addXdrMapping(xdr);
        }
        if (bme280Config.huAct){
            LOG_DEBUG(GwLog::DEBUG,"BME280 humidity active, adding capability and xdr mapping");
            bme280Active=true;
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRHUMIDITY;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_HUMIDITY_ACTUALHUMIDITY;
            xdr.selector=(int)bme280Config.huSrc;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=bme280Config.iid;
            xdr.xdrName=bme280Config.huNam;
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
        if (sht3xConfig.huAct || sht3xConfig.tmAct){
            sht3x=new SHT3X();
            sht3x->init(addr,&Wire);
            LOG_DEBUG(GwLog::LOG,"initialized SHT3X at address %d, intv %ld",(int)addr,sht3xConfig.intv);
            runLoop=true;
            timers.addAction(sht3xConfig.intv,[logger,api,sht3x,sht3xConfig,counterId](){
                int rt=0;
                if ((rt=sht3x->get())==0){
                    double temp=sht3x->cTemp;
                    temp=CToKelvin(temp);
                    double humid=sht3x->humidity;
                    LOG_DEBUG(GwLog::DEBUG,"SHT3X measure temp=%2.1f, humid=%2.0f",(float)temp,(float)humid);
                    tN2kMsg msg;
                    if (sht3xConfig.huAct){
                        SetN2kHumidity(msg,1,sht3xConfig.iid,sht3xConfig.huSrc,humid);
                        api->sendN2kMessage(msg);
                        api->increment(counterId,"SHT3Xhum");
                    }
                    if (sht3xConfig.tmAct){
                        SetN2kTemperature(msg,1,sht3xConfig.iid,sht3xConfig.tmSrc,temp);
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
        if (qmp6988Config.prAct){
            runLoop=true;
            qmp6988=new QMP6988();
            qmp6988->init(qaddr,&Wire);
            LOG_DEBUG(GwLog::LOG,"initialized QMP6988 at address %d, intv %ld",qaddr,qmp6988Config.intv);
            timers.addAction(qmp6988Config.intv,[logger,api,qmp6988,qmp6988Config,counterId](){
                float pressure=qmp6988->calcPressure();
                float computed=pressure+qmp6988Config.prOff;
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
        if (bme280Config.tmAct || bme280Config.prAct|| bme280Config.huAct){
            Adafruit_BME280 *bme280=new Adafruit_BME280();
            if (bme280->begin(baddr,&Wire)){
                uint32_t sensorId=bme280->sensorID();
                bool hasHumidity=sensorId == 0x60; //BME280, else BMP280
                if (bme280Config.tmOff != 0){
                    bme280->setTemperatureCompensation(bme280Config.tmOff);
                }
                if (hasHumidity || bme280Config.tmAct || bme280Config.prAct)
                {
                    LOG_DEBUG(GwLog::LOG, "initialized BME280 at %d, sensorId 0x%x", baddr, sensorId);
                    timers.addAction(bme280Config.intv, [logger, api, bme280, bme280Config, counterId, hasHumidity](){
                        if (bme280Config.prAct){
                            float pressure=bme280->readPressure();
                            float computed=pressure+bme280Config.prOff;
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure %2.0fPa, computed %2.0fPa",pressure,computed);
                            tN2kMsg msg;
                            SetN2kPressure(msg,1,bme280Config.iid,bme280Config.prSrc,computed);
                            api->sendN2kMessage(msg);
                            api->increment(counterId,"BME280press");
                        }
                        if (bme280Config.tmAct){
                            float temperature=bme280->readTemperature(); //offset is handled internally
                            temperature=CToKelvin(temperature);
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure temp=%2.1f",temperature);
                            tN2kMsg msg;
                            SetN2kTemperature(msg,1,bme280Config.iid,bme280Config.tmSrc,temperature);
                            api->sendN2kMessage(msg);
                            api->increment(counterId,"BME280temp");
                        }
                        if (bme280Config.huAct && hasHumidity){
                            float humidity=bme280->readHumidity();
                            LOG_DEBUG(GwLog::DEBUG,"BME280 read humid=%02.0f",humidity);
                            tN2kMsg msg; 
                            SetN2kHumidity(msg,1,bme280Config.iid,bme280Config.huSrc,humidity);
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
