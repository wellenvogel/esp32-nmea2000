#include "GwSHT3X.h"
#ifdef _GWSHT3X
class SHT3XConfig;
static GwSensorConfigInitializerList<SHT3XConfig> configs;
class SHT3XConfig : public IICSensorBase{
    public:
    String tmNam;
    String huNam;
    bool tmAct=false;
    bool huAct=false;
    tN2kHumiditySource huSrc;
    tN2kTempSource tmSrc;
    SHT3X *device=nullptr;
    using IICSensorBase::IICSensorBase;
    virtual bool isActive(){
        return tmAct || huAct;
    }
    virtual bool initDevice(GwApi * api,TwoWire *wire){
        if (! isActive()) return false;
        device=new SHT3X();
        device->init(addr,wire);
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"initialized %s at address %d, intv %ld",prefix.c_str(),(int)addr,intv);
        return true;
    }
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"%s configured",prefix.c_str());
        api->addCapability(prefix,"true");
        addHumidXdr(api,*this);
        addTempXdr(api,*this);
        return isActive();
    }
    virtual void measure(GwApi * api,TwoWire *wire, int counterId)
    {
        if (!device)
            return;
        GwLog *logger=api->getLogger();    
        int rt = 0;
        if ((rt = device->get()) == 0)
        {
            double temp = device->cTemp;
            temp = CToKelvin(temp);
            double humid = device->humidity;
            LOG_DEBUG(GwLog::DEBUG, "%s measure temp=%2.1f, humid=%2.0f",prefix.c_str(), (float)temp, (float)humid);
            if (huAct)
            {
                sendN2kHumidity(api, *this, humid, counterId);
            }
            if (tmAct)
            {
                sendN2kTemperature(api, *this, temp, counterId);
            }
        }
        else
        {
            LOG_DEBUG(GwLog::DEBUG, "unable to query %s: %d",prefix.c_str(), rt);
        }
    }
    
    virtual void readConfig(GwConfigHandler *cfg){
        if (ok) return;
        configs.readConfig(this,cfg);
        return;
    }
};
SensorBase::Creator creator=[](GwApi *api,const String &prfx)-> SensorBase*{
    if (! configs.knowsPrefix(prfx)) return nullptr;
    return new SHT3XConfig(api,prfx);
};
SensorBase::Creator registerSHT3X(GwApi *api){
    GwLog *logger=api->getLogger();
    #if defined(GWSHT3X) || defined (GWSHT3X11)
    {
        api->addSensor(creator(api,"SHT3X11"));
        CHECK_IIC1();
        #pragma message "GWSHT3X11 defined"
    }
    #endif
    #if defined(GWSHT3X12)
    {
        api->addSensor(creator(api,"SHT3X12"));
        CHECK_IIC1();
        #pragma message "GWSHT3X12 defined"
    }
    #endif
    #if defined(GWSHT3X21)
    {
        api->addSensor(creator(api,"SHT3X21"));
        CHECK_IIC2();
        #pragma message "GWSHT3X21 defined"
    }
    #endif
    #if defined(GWSHT3X22)
    {
        api->addSensor(creator(api,"SHT3X22"));
        CHECK_IIC2();
        #pragma message "GWSHT3X22 defined"
    }
    #endif
    return creator;
};

/**
     * we do not dynamically compute the config names
     * just to get compile time errors if something does not fit
     * correctly
    */
    #define CFGSHT3X(s,prefix,bus,baddr) \
        CFG_SGET(s,tmNam,prefix); \
        CFG_SGET(s,huNam,prefix); \
        CFG_SGET(s,iid,prefix); \
        CFG_SGET(s,tmAct,prefix); \
        CFG_SGET(s,huAct,prefix); \
        CFG_SGET(s,intv,prefix); \
        CFG_SGET(s,huSrc,prefix); \
        CFG_SGET(s,tmSrc,prefix); \
        s->busId = bus;                       \
        s->addr = baddr;                      \
        s->ok = true;  \
        s->intv*=1000;

#define SCSHT3X(list, prefix, bus, addr) \
    GWSENSORCONFIG(list, SHT3XConfig, prefix, [](SHT3XConfig *s, GwConfigHandler *cfg) { CFGSHT3X(s, prefix, bus, addr); });

SCSHT3X(configs, SHT3X11,1, 0x44);
SCSHT3X(configs, SHT3X12,1, 0x45);
SCSHT3X(configs, SHT3X21,2, 0x44);
SCSHT3X(configs, SHT3X22,2, 0x45);

#else
SensorBase::Creator registerSHT3X(GwApi *api){
    return SensorBase::Creator();
}

#endif



