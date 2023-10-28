#include "GwIicTask.h"
#include "GwHardware.h"
#include <Wire.h>
#include "SHT3X.h"
#include "QMP6988.h"
#include "GwTimer.h"
#include "N2kMessages.h"
#include "GwHardware.h"
#include "GwXdrTypeMappings.h"
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
void runIicTask(GwApi *api);
void initIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        return;
    #endif
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
    SHT3X *sht3x=nullptr;
    bool runLoop=false;
    GwIntervalRunner timers;
    int counterId=api->addCounter("iicsensors");
    #ifdef GWSHT3X
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
}
