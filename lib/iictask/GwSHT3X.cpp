#include "GwSHT3X.h"
#ifdef _GWSHT3X
#define PRFX1 "SHT3X11"
#define PRFX2 "SHT3X12"
#define PRFX3 "SHT3X21"
#define PRFX4 "SHT3X22"

class SHT3XConfig : public IICSensorBase{
    public:
    String tmNam;
    String huNam;
    bool tmAct=false;
    bool huAct=false;
    tN2kHumiditySource huSrc;
    tN2kTempSource tmSrc;
    SHT3X *device=nullptr;
    SHT3XConfig(GwApi *api,const String &prefix):
        SensorBase(api,prefix){}
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
    /**
     * we do not dynamically compute the config names
     * just to get compile time errors if something does not fit
     * correctly
    */
    #define CFG3X(prefix) \
        CFG_GET(tmNam,prefix); \
        CFG_GET(huNam,prefix); \
        CFG_GET(iid,prefix); \
        CFG_GET(tmAct,prefix); \
        CFG_GET(huAct,prefix); \
        CFG_GET(intv,prefix); \
        CFG_GET(huSrc,prefix); \
        CFG_GET(tmSrc,prefix);

    virtual void readConfig(GwConfigHandler *cfg){
        if (ok) return;
        if (prefix == PRFX1){
            busId=1;
            addr=0x44;
            CFG3X(SHT3X11);
            ok=true; 
        }
        if (prefix == PRFX2){
            busId=1;
            addr=0x45;
            CFG3X(SHT3X12);
            ok=true;
        }
        if (prefix == PRFX3){
            busId=2;
            addr=0x44;
            CFG3X(SHT3X21);
            ok=true;
        }
        if (prefix == PRFX4){
            busId=2;
            addr=0x45;
            CFG3X(SHT3X22);
            ok=true;
        }
        intv*=1000;
    }
};
IICSensorBase::Creator creator=[](GwApi *api,const String &prfx){
    return new SHT3XConfig(api,prfx);
};
IICSensorBase::Creator registerSHT3X(GwApi *api,IICSensorList &sensors){
    GwLog *logger=api->getLogger();
    #if defined(GWSHT3X) || defined (GWSHT3X11)
    {
        auto *scfg=creator(api,PRFX1);
        sensors.add(api,scfg);
        CHECK_IIC1();
        #pragma message "GWSHT3X11 defined"
    }
    #endif
    #if defined(GWSHT3X12)
    {
        auto *scfg=creator(api,PRFX2);
        sensors.add(api,scfg);
        CHECK_IIC1();
        #pragma message "GWSHT3X12 defined"
    }
    #endif
    #if defined(GWSHT3X21)
    {
        auto *scfg=creator(api,PRFX3);
        sensors.add(api,scfg);
        CHECK_IIC2();
        #pragma message "GWSHT3X21 defined"
    }
    #endif
    #if defined(GWSHT3X22)
    {
        auto *scfg=creator(api,PRFX4);
        sensors.add(api,scfg);
        CHECK_IIC2();
        #pragma message "GWSHT3X22 defined"
    }
    #endif
    return creator;
};

#else
IICSensorBase::Creator registerSHT3X(GwApi *api,IICSensorList &sensors){

}

#endif



