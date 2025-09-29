#include "GwSHTXX.h"
#if defined(_GWSHT3X) || defined(_GWSHT4X)
class SHTXXConfig : public IICSensorBase{
    public:
    String tmNam;
    String huNam;
    bool tmAct=false;
    bool huAct=false;
    bool sEnv=true;
    tN2kHumiditySource huSrc;
    tN2kTempSource tmSrc;
    using IICSensorBase::IICSensorBase;
    virtual bool isActive(){
        return tmAct || huAct;
    }
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"%s configured",prefix.c_str());
        addHumidXdr(api,*this);
        addTempXdr(api,*this);
        return isActive();
    }
    virtual bool doMeasure(GwApi * api,double &temp, double &humid){
        return false;
    }
    virtual void measure(GwApi * api,TwoWire *wire, int counterId) override
    {
        GwLog *logger=api->getLogger();    
        double temp = N2kDoubleNA;
        double humid = N2kDoubleNA;
        if (doMeasure(api,temp,humid)){
            temp = CToKelvin(temp);
            LOG_DEBUG(GwLog::DEBUG, "%s measure temp=%2.1f, humid=%2.0f",prefix.c_str(), (float)temp, (float)humid);
            if (huAct)
            {
                sendN2kHumidity(api, *this, humid, counterId);
            }
            if (tmAct)
            {
                sendN2kTemperature(api, *this, temp, counterId);
            }
            if (huAct || tmAct){
                sendN2kEnvironmentalParameters(api,*this,temp,humid,N2kDoubleNA,counterId);   
            }
        }
    }
    
};
/**
 * we do not dynamically compute the config names
 * just to get compile time errors if something does not fit
 * correctly
 */
#define INITSHTXX(type,prefix,bus,baddr) \
[] (type *s ,GwConfigHandler *cfg) { \
    CFG_SGET(s, tmNam, prefix); \
    CFG_SGET(s, huNam, prefix); \
    CFG_SGET(s, iid, prefix);   \
    CFG_SGET(s, tmAct, prefix); \
    CFG_SGET(s, huAct, prefix); \
    CFG_SGET(s, intv, prefix); \
    CFG_SGET(s, huSrc, prefix); \
    CFG_SGET(s, tmSrc, prefix); \
    CFG_SGET(s, sEnv,prefix); \
    s->busId = bus; \
    s->addr = baddr; \
    s->ok = true; \
    s->intv *= 1000; \
}

#if defined(_GWSHT3X)
class SHT3XConfig;
static GwSensorConfigInitializerList<SHT3XConfig> configs3;
class SHT3XConfig : public SHTXXConfig{
    SHT3X *device=nullptr;
    public:
    using SHTXXConfig::SHTXXConfig;
    virtual bool initDevice(GwApi * api,TwoWire *wire)override{
        if (! isActive()) return false;
        device=new SHT3X();
        device->init(addr,wire);
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"initialized %s at address %d, intv %ld",prefix.c_str(),(int)addr,intv);
        return true;
    }
    virtual bool doMeasure(GwApi *api,double &temp, double &humid) override{
        if (!device)
            return false;
        int rt=0;
        GwLog *logger=api->getLogger();
        if ((rt = device->get()) == 0)
        {
            temp = device->cTemp;
            humid = device->humidity;
            return true;
        }
        else{
            LOG_DEBUG(GwLog::DEBUG, "unable to query %s: %d",prefix.c_str(), rt);
        }
        return false;
    } 
    virtual void readConfig(GwConfigHandler *cfg) override{
        if (ok) return;
        configs3.readConfig(this,cfg);
        return;
    }
};

SensorBase::Creator creator3=[](GwApi *api,const String &prfx)-> SensorBase*{
        if (! configs3.knowsPrefix(prfx)) return nullptr;
        return new SHT3XConfig(api,prfx);
    };
SensorBase::Creator registerSHT3X(GwApi *api){
    GwLog *logger=api->getLogger();
    #if defined(GWSHT3X) || defined (GWSHT3X11)
    {
        api->addSensor(creator3(api,"SHT3X11"));
        CHECK_IIC1();
        #pragma message "GWSHT3X11 defined"
    }
    #endif
    #if defined(GWSHT3X12)
    {
        api->addSensor(creator3(api,"SHT3X12"));
        CHECK_IIC1();
        #pragma message "GWSHT3X12 defined"
    }
    #endif
    #if defined(GWSHT3X21)
    {
        api->addSensor(creator3(api,"SHT3X21"));
        CHECK_IIC2();
        #pragma message "GWSHT3X21 defined"
    }
    #endif
    #if defined(GWSHT3X22)
    {
        api->addSensor(creator3(api,"SHT3X22"));
        CHECK_IIC2();
        #pragma message "GWSHT3X22 defined"
    }
    #endif
    return creator3;
};


#define SCSHT3X(prefix, bus, addr) \
    GwSensorConfigInitializer<SHT3XConfig> __initCFGSHT3X ## prefix \
        (configs3,GwSensorConfig<SHT3XConfig>(#prefix,INITSHTXX(SHT3XConfig,prefix,bus,addr)));

SCSHT3X(SHT3X11, 1, 0x44);
SCSHT3X(SHT3X12, 1, 0x45);
SCSHT3X(SHT3X21, 2, 0x44);
SCSHT3X(SHT3X22, 2, 0x45);

#endif
#if defined(_GWSHT4X)
class SHT4XConfig;
static GwSensorConfigInitializerList<SHT4XConfig> configs4;
class SHT4XConfig : public SHTXXConfig{
    SHT4X *device=nullptr;
    public:
    using SHTXXConfig::SHTXXConfig;
    virtual bool initDevice(GwApi * api,TwoWire *wire)override{
        if (! isActive()) return false;
        device=new SHT4X();
        device->begin(wire,addr);
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"initialized %s at address %d, intv %ld",prefix.c_str(),(int)addr,intv);
        return true;
    }
    virtual bool doMeasure(GwApi *api,double &temp, double &humid) override{
        if (!device)
            return false;
        GwLog *logger=api->getLogger();
        if (device->update())
        {
            temp = device->cTemp;
            humid = device->humidity;
            return true;
        }
        else{
            LOG_DEBUG(GwLog::DEBUG, "unable to query %s",prefix.c_str());
        }
        return false;
    } 
    virtual void readConfig(GwConfigHandler *cfg) override{
        if (ok) return;
        configs4.readConfig(this,cfg);
        return;
    }
};

SensorBase::Creator creator4=[](GwApi *api,const String &prfx)-> SensorBase*{
        if (! configs4.knowsPrefix(prfx)) return nullptr;
        return new SHT4XConfig(api,prfx);
    };
SensorBase::Creator registerSHT4X(GwApi *api){
    GwLog *logger=api->getLogger();
    #if defined(GWSHT4X) || defined (GWSHT4X11)
    {
        api->addSensor(creator3(api,"SHT4X11"));
        CHECK_IIC1();
        #pragma message "GWSHT4X11 defined"
    }
    #endif
    #if defined(GWSHT4X12)
    {
        api->addSensor(creator3(api,"SHT4X12"));
        CHECK_IIC1();
        #pragma message "GWSHT4X12 defined"
    }
    #endif
    #if defined(GWSHT4X21)
    {
        api->addSensor(creator3(api,"SHT4X21"));
        CHECK_IIC2();
        #pragma message "GWSHT4X21 defined"
    }
    #endif
    #if defined(GWSHT4X22)
    {
        api->addSensor(creator3(api,"SHT4X22"));
        CHECK_IIC2();
        #pragma message "GWSHT4X22 defined"
    }
    #endif
    return creator4;
};


#define SCSHT4X(prefix, bus, addr) \
    GwSensorConfigInitializer<SHT4XConfig> __initCFGSHT4X ## prefix \
        (configs4,GwSensorConfig<SHT4XConfig>(#prefix,INITSHTXX(SHT4XConfig,prefix,bus,addr)));

SCSHT4X(SHT4X11, 1, 0x44);
SCSHT4X(SHT4X12, 1, 0x45);
SCSHT4X(SHT4X21, 2, 0x44);
SCSHT4X(SHT4X22, 2, 0x45);
#endif 
#endif
#ifndef _GWSHT3X
SensorBase::Creator registerSHT3X(GwApi *api){
    return SensorBase::Creator();
}
#endif
#ifndef _GWSHT4X
SensorBase::Creator registerSHT4X(GwApi *api){
    return SensorBase::Creator();
}
#endif



