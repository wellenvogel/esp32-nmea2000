#ifndef _GWIICSENSSORS_H
#define _GWIICSENSSORS_H
#include "GwApi.h"
#include "N2kMessages.h"
#include "GwXdrTypeMappings.h"
#include "GwHardware.h"
#ifdef _GWIIC
    #include <Wire.h>
#endif

#define CFG_GET(name,prefix) \
    cfg->getValue(name, GwConfigDefinitions::prefix ## name)

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


class SensorBase{
    public:
    int busId=0;
    int iid=99; //N2K instanceId
    int addr=-1;
    String prefix;
    long intv=0;
    bool ok=false;
    virtual void readConfig(GwConfigHandler *cfg)=0;
    SensorBase(GwApi *api,const String &prfx):prefix(prfx){
    }
    virtual bool isActive(){return false;};
    virtual bool initDevice(GwApi *api,TwoWire *wire){return false;};
    virtual bool preinit(GwApi * api){return false;}
    virtual void measure(GwApi * api,TwoWire *wire, int counterId){};
    virtual ~SensorBase(){}
};

class SensorList : public std::vector<SensorBase*>{
    public:
    void add(GwApi *api, SensorBase *sensor){
        sensor->readConfig(api->getConfig());
        api->getLogger()->logDebug(GwLog::LOG,"configured sensor %s, status %d",sensor->prefix.c_str(),(int)sensor->ok);
        push_back(sensor);
    }
    using std::vector<SensorBase*>::vector;
};

#endif