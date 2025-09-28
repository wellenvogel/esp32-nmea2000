#include "GwIicTask.h"
class IICGrove
{
public:
    String base;
    String grove;
    String suffix;
    IICGrove(const String &b, const String &g, const String &s) : base(b), grove(g), suffix(s) {}
    String item(const String &grove, const String &bus)
    {
        if (grove == this->grove)
            return base + bus + suffix;
        return "";
    }
};
static std::vector<IICGrove> iicGroveList;
#define GROOVE_IIC(base, grove, suffix) \
    static GwInitializer<IICGrove> base##grove##suffix(iicGroveList, IICGrove(#base, #grove, #suffix));


#include "GwIicSensors.h"
#include "GwHardware.h"
#include "GwBME280.h"
#include "GwBMP280.h"
#include "GwQMP6988.h"
#include "GwSHT3X.h"
#include <map>

#include "GwTimer.h"

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

static void addGroveItems(std::vector<SensorBase::Creator> &creators,GwApi *api, const String &bus,const String &grove, int, int)
{
    GwLog *logger=api->getLogger();
    for (auto &&init : iicGroveList)
    {
        LOG_DEBUG(GwLog::DEBUG, "trying grove item %s:%s:%s for grove %s, bus %s", 
            init.base.c_str(),init.grove.c_str(),
            init.suffix.c_str(),grove.c_str(),bus.c_str()
            );
        String prfx = init.item(grove, bus);
        if (!prfx.isEmpty())
        {
            bool found=false;
            for (auto &&creator : creators)
            {
                if (! creator) continue;
                auto *scfg = creator(api, prfx);
                if (scfg == nullptr) continue;
                scfg->readConfig(api->getConfig());
                if (scfg->ok)
                {
                    LOG_DEBUG(GwLog::LOG, "adding %s from grove config", prfx.c_str());
                    api->addSensor(scfg,false);
                    found=true;
                    break;
                }
                else
                {
                    LOG_DEBUG(GwLog::DEBUG, "unmatched grove sensor config %s", prfx.c_str());
                    delete scfg;
                }
            }
            if (! found){
                LOG_DEBUG(GwLog::ERROR,"no iic sensor found for %s",prfx.c_str());
            }
        }
    }
}

void initIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        return;
    #else
    bool addTask=false;
    GwConfigHandler *config=api->getConfig();
    std::vector<SensorBase::Creator> creators;
    creators.push_back(registerSHT3X(api));
    creators.push_back(registerQMP6988(api));
    creators.push_back(registerBME280(api));
    creators.push_back(registerBMP280(api));
    #ifdef _GWI_IIC1
        addGroveItems(creators,api,"1",_GWI_IIC1);    
    #endif
    #ifdef _GWI_IIC2
        addGroveItems(creators,api,"2",_GWI_IIC2);    
    #endif
    //TODO: ensure that we run after other init tasks...
    int res=-1;
    ConfiguredSensors sensorList=api->taskInterfaces()->get<ConfiguredSensors>(res);
    for (auto &&it: sensorList.sensors){
        if (it->busType != SensorBase::IIC) continue;
        if (it->preinit(api)) {
            addTask=true;
            api->addCapability(it->prefix,"true");
        }
    }
    if (addTask){
        api->addUserTask(runIicTask,"iicTask",4000);
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
        return initWireDo(logger,wire,num,"",GWIIC_SCL,GWIIC_SDA);
    }
    if (num == 2){
        #ifdef _GWI_IIC2
            return initWireDo(logger,wire,num,_GWI_IIC2);
        #endif
        return initWireDo(logger,wire,num,"",GWIIC_SCL2,GWIIC_SDA2);
    }
    return false;
}
void runIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    std::map<int,TwoWire *> buses;
    LOG_DEBUG(GwLog::LOG,"iic task started");
    int res=-1;
    ConfiguredSensors sensorList=api->taskInterfaces()->get<ConfiguredSensors>(res);
    for (auto &&it : sensorList.sensors){
        if (it->busType != SensorBase::IIC) continue;
        int busId=it->busId;
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
                LOG_DEBUG(GwLog::ERROR, "invalid bus id %d at config %s", busId, it->prefix.c_str());
                break;
            }
        }
    }
    GwConfigHandler *config=api->getConfig();
    bool runLoop=false;
    GwIntervalRunner timers;
    int counterId=api->addCounter("iicsensors");
    for (auto && cfg: sensorList.sensors){
        if (cfg->busType != SensorBase::IIC) continue;
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
