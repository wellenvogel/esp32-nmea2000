#ifndef _GWIICSENSSORS_H
#define _GWIICSENSSORS_H
#include "GwApi.h"
#include "N2kMessages.h"
#include "GwXdrTypeMappings.h"
#include "GwHardware.h"
#include "GwSensor.h"
#ifdef _GWIIC
    #include <Wire.h>
#else
    class TwoWire;
#endif

using BUSTYPE=TwoWire;
using IICSensorList=SensorList;
using IICSensorBase=SensorTemplate<BUSTYPE,SensorBase::IIC>;


template <class CFG>
bool addPressureXdr(GwApi *api, CFG &cfg)
{
    if (! cfg.prAct) return false;
    if (cfg.prNam.isEmpty()){
        api->getLogger()->logDebug(GwLog::LOG, "pressure active for %s, no xdr mapping", cfg.prefix.c_str());    
        return true;
    }
    api->getLogger()->logDebug(GwLog::LOG, "adding pressure xdr mapping for %s", cfg.prefix.c_str());
    GwXDRMappingDef xdr;
    xdr.category = GwXDRCategory::XDRPRESSURE;
    xdr.direction = GwXDRMappingDef::M_FROM2K;
    xdr.selector = (int)cfg.prSrc;
    xdr.instanceId = cfg.iid;
    xdr.instanceMode = GwXDRMappingDef::IS_SINGLE;
    xdr.xdrName = cfg.prNam;
    api->addXdrMapping(xdr);
    return true;
}

template <class CFG>
bool addTempXdr(GwApi *api, CFG &cfg)
{
    if (!cfg.tmAct) return false;
    if (cfg.tmNam.isEmpty()){
        api->getLogger()->logDebug(GwLog::LOG, "temperature active for %s, no xdr mapping", cfg.prefix.c_str());    
        return true;
    }
    api->getLogger()->logDebug(GwLog::LOG, "adding temperature xdr mapping for %s", cfg.prefix.c_str());
    GwXDRMappingDef xdr;
    xdr.category = GwXDRCategory::XDRTEMP;
    xdr.direction = GwXDRMappingDef::M_FROM2K;
    xdr.field = GWXDRFIELD_TEMPERATURE_ACTUALTEMPERATURE;
    xdr.selector = (int)cfg.tmSrc;
    xdr.instanceMode = GwXDRMappingDef::IS_SINGLE;
    xdr.instanceId = cfg.iid;
    xdr.xdrName = cfg.tmNam;
    api->addXdrMapping(xdr);
    return true;
}

template <class CFG>
bool addHumidXdr(GwApi *api, CFG &cfg)
{
    if (! cfg.huAct) return false;
    if (cfg.huNam.isEmpty()){
        api->getLogger()->logDebug(GwLog::LOG, "humidity active for %s, no xdr mapping", cfg.prefix.c_str());    
        return true;
    }
    api->getLogger()->logDebug(GwLog::LOG, "adding humidity xdr mapping for %s", cfg.prefix.c_str());
    GwXDRMappingDef xdr;
    xdr.category = GwXDRCategory::XDRHUMIDITY;
    xdr.direction = GwXDRMappingDef::M_FROM2K;
    xdr.field = GWXDRFIELD_HUMIDITY_ACTUALHUMIDITY;
    xdr.selector = (int)cfg.huSrc;
    xdr.instanceMode = GwXDRMappingDef::IS_SINGLE;
    xdr.instanceId = cfg.iid;
    xdr.xdrName = cfg.huNam;
    api->addXdrMapping(xdr);
    return true;
}

template <class CFG>
void sendN2kHumidity(GwApi *api,CFG &cfg,double value, int counterId){
    tN2kMsg msg;
    SetN2kHumidity(msg,1,cfg.iid,cfg.huSrc,value);
    api->sendN2kMessage(msg);
    api->increment(counterId,cfg.prefix+String("hum"));
}

template <class CFG>
void sendN2kPressure(GwApi *api,CFG &cfg,double value, int counterId){
    tN2kMsg msg;
    SetN2kPressure(msg,1,cfg.iid,cfg.prSrc,value);
    api->sendN2kMessage(msg);
    api->increment(counterId,cfg.prefix+String("press"));
}

template <class CFG>
void sendN2kTemperature(GwApi *api,CFG &cfg,double value, int counterId){
    tN2kMsg msg;
    SetN2kTemperature(msg,1,cfg.iid,cfg.tmSrc,value);
    api->sendN2kMessage(msg);
    api->increment(counterId,cfg.prefix+String("temp"));
}

template <class CFG>
void sendN2kEnvironmentalParameters(GwApi *api,CFG &cfg,double tmValue, double huValue, double prValue, int counterId){
    if (! cfg.sEnv) return;
    tN2kMsg msg;
    SetN2kEnvironmentalParameters(msg,1,cfg.tmSrc,tmValue,cfg.huSrc,huValue,prValue);
    api->sendN2kMessage(msg);
    if (huValue != N2kDoubleNA){
        api->increment(counterId,cfg.prefix+String("ehum"));
    }
    if (prValue != N2kDoubleNA){
        api->increment(counterId,cfg.prefix+String("epress"));
    }
    if (tmValue != N2kDoubleNA){
        api->increment(counterId,cfg.prefix+String("etemp"));
    }
}

#ifndef _GWI_IIC1
    #define CHECK_IIC1() checkDef(GWIIC_SCL,GWIIC_SDA)
#else
    #define CHECK_IIC1()
#endif
#ifndef _GWI_IIC2
    #define CHECK_IIC2() checkDef(GWIIC_SCL2,GWIIC_SDA2)
#else
    #define CHECK_IIC2()
#endif

#endif
