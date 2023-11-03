#include "GwIicTask.h"
#include "GwHardware.h"
#include <map>
#ifdef _GWIIC
    #include <Wire.h>
    #if defined(GWSHT3X) || defined(GWSHT3X1) || defined(GWSHT3X2) || defined(GWSHT3X2) || defined(GWSHT3X4)
        #define _GWSHT3X
    #else
        #undef _GWSHT3X
    #endif
    #if defined(GWQMP6988) || defined(GWQMP69881) || defined(GWQMP69882) || defined(GWQMP69883) || defined(GWQMP69884)
        #define _GWQMP6988
    #else
        #undef _GWQMP6988
    #endif
#else
    #undef _GWSHT3X
    #undef GWSHT3X
    #undef GWSHT3X1
    #undef GWSHT3X2
    #undef GWSHT3X3
    #undef GWSHT3X4
    #undef _GWQMP6988
    #undef GWQMP6988
    #undef GWQMP69881
    #undef GWQMP69882
    #undef GWQMP69883
    #undef GWQMP69884
#endif
#ifdef _GWSHT3X
    #include "SHT3X.h"
#endif
#ifdef _GWQMP6988
    #include "QMP6988.h"
#endif
#include "GwTimer.h"
#include "N2kMessages.h"
#include "GwHardware.h"
#include "GwXdrTypeMappings.h"
#ifdef GWBME280
    #include <Adafruit_BME280.h>
#endif


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
    virtual void readConfig(GwConfigHandler *cfg){};
    SensorBase(GwApi *api,const String &prfx):prefix(prfx){
        readConfig(api->getConfig());
    }
    virtual bool isActive(){return false;};
    virtual bool initDevice(GwApi *api,TwoWire *wire){return false;};
    virtual bool preinit(GwApi * api){return false;}
    virtual void measure(GwApi * api,TwoWire *wire, int counterId){};
    virtual ~SensorBase(){}
};

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

#define CFG_GET(name,prefix) \
    cfg->getValue(name, GwConfigDefinitions::prefix ## name)

#define CBME280(name) \
    CFG_GET(name,BME2801)    

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
        }
        intv*=1000;
    }
};


#endif

#ifdef _GWQMP6988
class QMP6988Config : public SensorBase{
    public:
        String prNam="Pressure";
        bool prAct=true;
        tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
        float prOff=0;
        QMP6988 *device=nullptr;
        QMP6988Config(GwApi* api,const String &prefix):SensorBase(api,prefix){}
        virtual bool isActive(){return prAct;};
        virtual bool initDevice(GwApi *api,TwoWire *wire){
            if (!isActive()) return false;
            GwLog *logger=api->getLogger(); 
            device=new QMP6988();
            if (!device->init(addr,wire)){
                LOG_DEBUG(GwLog::ERROR,"unable to initialize %s at address %d, intv %ld",prefix.c_str(),addr,intv);
                delete device;
                device=nullptr;
                return false;
            }
            LOG_DEBUG(GwLog::LOG,"initialized %s at address %d, intv %ld",prefix.c_str(),addr,intv);
            return true;
        };
        virtual bool preinit(GwApi * api){
            GwLog *logger=api->getLogger();
            LOG_DEBUG(GwLog::LOG,"QMP6988 configured");
            api->addCapability(prefix,"true");
            addPressureXdr(api,*this);
            return isActive();
        }
        virtual void measure(GwApi * api,TwoWire *wire, int counterId){
            GwLog *logger=api->getLogger();
            float pressure=device->calcPressure();
            float computed=pressure+prOff;
            LOG_DEBUG(GwLog::DEBUG,"%s measure %2.0fPa, computed %2.0fPa",prefix.c_str(), pressure,computed);
            sendN2kPressure(api,*this,computed,counterId);
        }
        virtual void readConfig(GwConfigHandler *cfg){
            if (prefix == "QMP69881"){
                busId=1;
                addr=86;
                #undef CG
                #define CG(name) CFG_GET(name,QMP69881)
                CG(prNam);
                CG(iid);
                CG(prAct);
                CG(intv);
                CG(prOff);
            }
            if (prefix == "QMP69882"){
                busId=1;
                addr=112;
                #undef CG
                #define CG(name) CFG_GET(name,QMP69882)
                CG(prNam);
                CG(iid);
                CG(prAct);
                CG(intv);
                CG(prOff);
            }
            if (prefix == "QMP69883"){
                busId=2;
                addr=86;
                #undef CG
                #define CG(name) CFG_GET(name,QMP69883)
                CG(prNam);
                CG(iid);
                CG(prAct);
                CG(intv);
                CG(prOff);
            }
            if (prefix == "QMP69884"){
                busId=2;
                addr=112;
                #undef CG
                #define CG(name) CFG_GET(name,QMP69884)
                CG(prNam);
                CG(iid);
                CG(prAct);
                CG(intv);
                CG(prOff);
            }
            intv*=1000;

        }
};
#endif

class BME280Config{
    public:
    const String prefix="BME2801";
    bool prAct=true;
    bool tmAct=true;
    bool huAct=true;
    tN2kTempSource tmSrc=tN2kTempSource::N2kts_InsideTemperature;
    tN2kHumiditySource huSrc=tN2kHumiditySource::N2khs_InsideHumidity;
    tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
    int iid=99;
    long intv=2000;
    String tmNam="Temperature";
    String huNam="Humidity";
    String prNam="Pressure";
    float tmOff=0;
    float prOff=0;
    BME280Config(GwConfigHandler *cfg){
        CBME280(prAct);
        CBME280(tmAct);
        CBME280(huAct);
        CBME280(tmSrc);
        CBME280(huSrc);
        CBME280(iid);
        CBME280(intv);
        intv*=1000;
        CBME280(tmNam);
        CBME280(huNam);
        CBME280(prNam);
        CBME280(tmOff);
        CBME280(prOff);
    }
};


void runIicTask(GwApi *api);

static std::vector<SensorBase*> sensors;

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
        sensors.push_back(scfg);
    }
    #endif
    #if defined(GWSHT3X2)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,"SHT3X2");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.push_back(scfg);
    }
    #endif
    #if defined(GWSHT3X3)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,"SHT3X3");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.push_back(scfg);
    }
    #endif
    #if defined(GWSHT3X4)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,"SHT3X4");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.push_back(scfg);
    }
    #endif
    #if defined(GWQMP6988) || defined(GWQMP69881)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69881");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.push_back(scfg);
    }
    #endif
    #if defined(GWQMP69882)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69882");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.push_back(scfg);
    }
    #endif
    #if defined(GWQMP69883)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69883");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.push_back(scfg);
    }
    #endif
    #if defined(GWQMP69884)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69884");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.push_back(scfg);
    }
    #endif
    #ifdef GWBME280
        LOG_DEBUG(GwLog::LOG,"BME280 configured");
        BME280Config bme280Config(api->getConfig());
        api->addCapability(bme280Config.prefix,"true");
        bool bme280Active=false;
        if (addPressureXdr(api,bme280Config)) bme280Active=true;
        if (addTempXdr(api,bme280Config)) bme280Active=true;
        if (addHumidXdr(api,bme280Config)) bme280Active=true;
        if (! bme280Active){
            LOG_DEBUG(GwLog::DEBUG,"BME280 configured but disabled");
        }
        else{
            addTask=true;
        }
    #endif
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
            break;
            case 2:
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
    #ifdef GWBME280
        int baddr=GWBME280;
        if (baddr < 0) baddr=0x76;
        BME280Config bme280Config(api->getConfig());
        if (bme280Config.tmAct || bme280Config.prAct|| bme280Config.huAct){
            Adafruit_BME280 *bme280=new Adafruit_BME280();
            if (bme280->begin(baddr,&Wire)){
                uint32_t sensorId=bme280->sensorID();
                bool hasHumidity=sensorId == 0x60; //BME280, else BMP280
                if (bme280Config.tmOff != 0){
                    bme280->setTemperatureCompensation(bme280Config.tmOff);
                }
                if (hasHumidity || bme280Config.tmAct || bme280Config.prAct)
                {
                    LOG_DEBUG(GwLog::LOG, "initialized BME280 at %d, sensorId 0x%x", baddr, sensorId);
                    timers.addAction(bme280Config.intv, [logger, api, bme280, bme280Config, counterId, hasHumidity](){
                        if (bme280Config.prAct){
                            float pressure=bme280->readPressure();
                            float computed=pressure+bme280Config.prOff;
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure %2.0fPa, computed %2.0fPa",pressure,computed);
                            sendN2kPressure(api,bme280Config,computed,counterId);
                        }
                        if (bme280Config.tmAct){
                            float temperature=bme280->readTemperature(); //offset is handled internally
                            temperature=CToKelvin(temperature);
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure temp=%2.1f",temperature);
                            sendN2kTemperature(api,bme280Config,temperature,counterId);
                        }
                        if (bme280Config.huAct && hasHumidity){
                            float humidity=bme280->readHumidity();
                            LOG_DEBUG(GwLog::DEBUG,"BME280 read humid=%02.0f",humidity);
                            sendN2kHumidity(api,bme280Config,humidity,counterId);
                        }
                    });
                    runLoop = true;
                }
                else{
                    LOG_DEBUG(GwLog::ERROR,"BME280 only humidity active, but sensor does not have it");
                }
            }
            else{
                LOG_DEBUG(GwLog::ERROR,"unable to initialize BME280 sensor at address %d",baddr);
            }
        }
    #endif
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
