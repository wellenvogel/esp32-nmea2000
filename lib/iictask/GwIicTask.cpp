#include "GwIicTask.h"
#include "GwIicSensors.h"
#include "GwHardware.h"
#include "GwBME280.h"
#include "GwQMP6988.h"
#include "GwSHT3X.h"
#include <map>

#include "GwTimer.h"
#include "GwHardware.h"

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

void runIicTask(GwApi *api);

static IICSensorList sensors;

void initIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        return;
    #else
    bool addTask=false;
    GwConfigHandler *config=api->getConfig();
    registerSHT3X(api,sensors);
    registerQMP6988(api,sensors);
    registerBME280(api,sensors);
    for (auto it=sensors.begin();it != sensors.end();it++){
        if ((*it)->preinit(api)) addTask=true;
    }
    if (addTask){
        api->addUserTask(runIicTask,"iicTask",3000);
    }
    #endif
}
#ifndef _GWIIC 
void runIicTask(GwApi *api){
   GwLog *logger=api->getLogger();
   LOG_DEBUG(GwLog::LOG,"no iic defined, iic task stopped");
   vTaskDelete(NULL);
   return; 
}
#else
bool initWireDo(GwLog *logger, TwoWire &wire, int num, const String &dummy, int scl, int sda)
{
    if (sda < 0 || scl < 0)
    {
        LOG_DEBUG(GwLog::ERROR, "IIC %d invalid config sda=%d,scl=%d",
                  num, sda, scl);
        return false;
    }
    bool rt = Wire.begin(sda, scl);
    if (!rt)
    {
        LOG_DEBUG(GwLog::ERROR, "unable to initialize IIC %d at sad=%d,scl=%d",
                  num, sda, scl);
        return rt;
    }
    LOG_DEBUG(GwLog::ERROR, "initialized IIC %d at sda=%d,scl=%d",
                                  num,sda,scl);
    return true;                                
}
bool initWire(GwLog *logger, TwoWire &wire, int num){
    if (num == 1){
        #ifdef _GWI_IIC1
            return initWireDo(logger,wire,num,_GWI_IIC1);
        #endif
        return initWireDo(logger,wire,num,"",GWIIC_SDA,GWIIC_SCL);
    }
    if (num == 2){
        #ifdef _GWI_IIC2
            return initWireDo(logger,wire,num,_GWI_IIC2);
        #endif
        return initWireDo(logger,wire,num,"",GWIIC_SDA2,GWIIC_SCL2);
    }
}
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
                if (initWire(logger,Wire,1)){
                    buses[busId] = &Wire;
                }
            }
            break;
            case 2:
            {
                if (initWire(logger,Wire1,2)){
                    buses[busId] = &Wire1;
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
        IICSensorBase *cfg=*it;
        auto bus=buses.find(cfg->busId);
        if (! cfg->isActive()) continue;
        if (bus == buses.end()){
            LOG_DEBUG(GwLog::ERROR,"No bus initialized for %s",cfg->prefix.c_str());
            continue;
        }
        TwoWire *wire=bus->second;
        bool rt=cfg->initDevice(api,wire);
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
}
#endif
