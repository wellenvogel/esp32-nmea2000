#include "GwIicTask.h"
#include "GwIicSensors.h"
#include "GwHardware.h"
#include "GwBME280.h"
#include "GwQMP6988.h"
#include <map>
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

#ifndef GWIIC_SDA
    #define GWIIC_SDA -1
#endif
#ifndef GWIIC_SCL
    #define GWIIC_SCL -1
#endif
#ifndef GWIIC_SDA2
    #define GWIIC_SDA2 -1
#endif
#ifndef GWIIC_SCL2
    #define GWIIC_SCL2 -1
#endif

#include "GwTimer.h"
#include "GwHardware.h"

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


#endif




void runIicTask(GwApi *api);

static SensorList sensors;

void initIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        vTaskDelete(NULL);
        return;
    #else
    bool addTask=false;
    GwConfigHandler *config=api->getConfig();
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
    registerQMP6988(api,sensors);
    registerBME280(api,sensors);
    for (auto it=sensors.begin();it != sensors.end();it++){
        if ((*it)->preinit(api)) addTask=true;
    }
    if (addTask){
        api->addUserTask(runIicTask,"iicTask",3000);
    }
}
#ifndef _GWIIC 
void runIicTask(GwApi *api){
   GwLog *logger=api->getLogger();
   LOG_DEBUG(GwLog::LOG,"no iic defined, iic task stopped");
   vTaskDelete(NULL);
   return; 
}
#else
void runIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    std::map<int,TwoWire *> buses;
    LOG_DEBUG(GwLog::LOG,"iic task started");
    for (auto it=sensors.begin();it != sensors.end();it++){
        int busId=(*it)->busId;
        auto bus=buses.find(busId);
        if (bus == buses.end()){
            switch (busId)
            {
            case 1:
            {
                if (GWIIC_SDA < 0 || GWIIC_SCL < 0)
                {
                    LOG_DEBUG(GwLog::ERROR, "IIC 1 invalid config %d,%d",
                              (int)GWIIC_SDA, (int)GWIIC_SCL);
                }
                else
                {
                    bool rt = Wire.begin(GWIIC_SDA, GWIIC_SCL);
                    if (!rt)
                    {
                        LOG_DEBUG(GwLog::ERROR, "unable to initialize IIC 1 at %d,%d",
                                  (int)GWIIC_SDA, (int)GWIIC_SCL);
                    }
                    else
                    {
                        buses[busId] = &Wire;
                    }
                }
            }
            break;
            case 2:
            {
                if (GWIIC_SDA2 < 0 || GWIIC_SCL2 < 0)
                {
                    LOG_DEBUG(GwLog::ERROR, "IIC 2 invalid config %d,%d",
                              (int)GWIIC_SDA2, (int)GWIIC_SCL2);
                }
                else
                {

                    bool rt = Wire1.begin(GWIIC_SDA2, GWIIC_SCL2);
                    if (!rt)
                    {
                        LOG_DEBUG(GwLog::ERROR, "unable to initialize IIC 2 at %d,%d",
                                  (int)GWIIC_SDA2, (int)GWIIC_SCL2);
                    }
                    else
                    {
                        buses[busId] = &Wire1;
                    }
                }
            }
            break;
            default:
                LOG_DEBUG(GwLog::ERROR, "invalid bus id %d at config %s", busId, (*it)->prefix.c_str());
                break;
            }
        }
    }
    GwConfigHandler *config=api->getConfig();
    bool runLoop=false;
    GwIntervalRunner timers;
    int counterId=api->addCounter("iicsensors");
    for (auto it=sensors.begin();it != sensors.end();it++){
        SensorBase *cfg=*it;
        auto bus=buses.find(cfg->busId);
        if (! cfg->isActive()) continue;
        if (bus == buses.end()){
            LOG_DEBUG(GwLog::ERROR,"No bus initialized for %s",cfg->prefix.c_str());
            continue;
        }
        TwoWire *wire=bus->second;
        bool rt=cfg->initDevice(api,&Wire);
        if (rt){
            runLoop=true;
            timers.addAction(cfg->intv,[wire,api,cfg,counterId](){
                cfg->measure(api,wire,counterId);
            });
        }
    }
    
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
#endif
