#include "GwIicTask.h"
#include "GwIicSensors.h"
#include "GwHardware.h"
#include "GwBME280.h"
#include "GwQMP6988.h"
#include "GwSHT3X.h"
#include <map>

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
    registerSHT3X(api,sensors);
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
