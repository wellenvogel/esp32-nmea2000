#include "GwSHT3X.h"
#ifdef _GWIIC
    #if defined(GWSHT3X) || defined(GWSHT3X1) || defined(GWSHT3X2) || defined(GWSHT3X2) || defined(GWSHT3X4)
        #define _GWSHT3X
    #else
        #undef _GWSHT3X
    #endif
#else
    #undef _GWSHT3X
    #undef GWSHT3X
    #undef GWSHT3X1
    #undef GWSHT3X2
    #undef GWSHT3X3
    #undef GWSHT3X4
#endif
#ifdef _GWSHT3X
    #include "SHT3X.h"
#endif

#ifdef _GWSHT3X
class SHT3XConfig : public SensorBase{
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
        LOG_DEBUG(GwLog::LOG,"SHT3X configured");
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
            LOG_DEBUG(GwLog::DEBUG, "SHT3X measure temp=%2.1f, humid=%2.0f", (float)temp, (float)humid);
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
            LOG_DEBUG(GwLog::DEBUG, "unable to query SHT3X: %d", rt);
        }
    }
    virtual void readConfig(GwConfigHandler *cfg){
        if (prefix == "SHT3X1"){
            busId=1;
            addr=0x44;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X1)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc);
            ok=true; 
        }
        if (prefix == "SHT3X2"){
            busId=1;
            addr=0x45;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X2)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc); 
            ok=true;
        }
        if (prefix == "SHT3X3"){
            busId=2;
            addr=0x44;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X3)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc); 
            ok=true;
        }
        if (prefix == "SHT3X4"){
            busId=2;
            addr=0x45;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X4)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc); 
            ok=true;
        }
        intv*=1000;
    }
};
void registerSHT3X(GwApi *api,SensorList &sensors){
    GwLog *logger=api->getLogger();
    #if defined(GWSHT3X) || defined (GWSHT3X1)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,"SHT3X1");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWSHT3X2)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,"SHT3X2");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWSHT3X3)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,"SHT3X3");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWSHT3X4)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,"SHT3X4");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
}
#else
void registerSHT3X(GwApi *api,SensorList &sensors){

}

#endif



