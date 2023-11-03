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
    #if defined(GWBME280) || defined(GWBME2801) || defined(GWBME2802)|| defined(GWBME2803)|| defined(GWBME2804)
        #define _GWBME280
    #else
        #undef _GWBME280
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
    #undef _GWBME280
    #undef GWBME280
    #undef GWBME2801
    #undef GWBME2802
    #undef GWBME2803
    #undef GWBME2804
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
#ifdef _GWBME280
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
        push_back(sensor);
    }
    using std::vector<SensorBase*>::vector;
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
                ok=true;
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
                ok=true;
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
                ok=true;
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
                ok=true;
            }
            intv*=1000;

        }
};
#endif

#ifdef _GWBME280
class BME280Config : public SensorBase{
    public:
    bool prAct=true;
    bool tmAct=true;
    bool huAct=true;
    tN2kTempSource tmSrc=tN2kTempSource::N2kts_InsideTemperature;
    tN2kHumiditySource huSrc=tN2kHumiditySource::N2khs_InsideHumidity;
    tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
    String tmNam="Temperature";
    String huNam="Humidity";
    String prNam="Pressure";
    float tmOff=0;
    float prOff=0;
    Adafruit_BME280 *device=nullptr;
    uint32_t sensorId=-1;
    BME280Config(GwApi * api, const String &prfx):SensorBase(api,prfx){
        }
    virtual bool isActive(){return prAct||huAct||tmAct;}
    virtual bool initDevice(GwApi *api,TwoWire *wire){
        GwLog *logger=api->getLogger();
        device= new Adafruit_BME280();
        if (! device->begin(addr,wire)){
            LOG_DEBUG(GwLog::ERROR,"unable to initialize %s at %d",prefix.c_str(),addr);
            delete device;
            device=nullptr;
            return false;
        }
        if (tmOff != 0){
            device->setTemperatureCompensation(tmOff);
        }
        sensorId=device->sensorID();
        LOG_DEBUG(GwLog::LOG, "initialized %s at %d, sensorId 0x%x", prefix.c_str(), addr, sensorId);
        return (huAct &&  sensorId == 0x60) || tmAct || prAct;
    }
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"%s configured",prefix.c_str());
        api->addCapability(prefix,"true");
        addPressureXdr(api,*this);
        addTempXdr(api,*this);
        addHumidXdr(api,*this);
        return isActive();
    }
    virtual void measure(GwApi *api, TwoWire *wire, int counterId)
    {
        if (!device)
            return;
        GwLog *logger = api->getLogger();
        if (prAct)
        {
            float pressure = device->readPressure();
            float computed = pressure + prOff;
            LOG_DEBUG(GwLog::DEBUG, "%s measure %2.0fPa, computed %2.0fPa", prefix.c_str(), pressure, computed);
            sendN2kPressure(api, *this, computed, counterId);
        }
        if (tmAct)
        {
            float temperature = device->readTemperature(); // offset is handled internally
            temperature = CToKelvin(temperature);
            LOG_DEBUG(GwLog::DEBUG, "%s measure temp=%2.1f", prefix.c_str(), temperature);
            sendN2kTemperature(api, *this, temperature, counterId);
        }
        if (huAct && sensorId == 0x60)
        {
            float humidity = device->readHumidity();
            LOG_DEBUG(GwLog::DEBUG, "%s read humid=%02.0f", prefix.c_str(), humidity);
            sendN2kHumidity(api, *this, humidity, counterId);
        }
    }
    virtual void readConfig(GwConfigHandler *cfg) override
    {
        if (prefix == "BME2801")
        {
            busId = 1;
            addr = 0x76;
            #undef CG
            #define CG(name) CFG_GET(name, BME2801)
            CG(prAct);
            CG(tmAct);
            CG(huAct);
            CG(tmSrc);
            CG(huSrc);
            CG(iid);
            CG(intv);
            CG(tmNam);
            CG(huNam);
            CG(prNam);
            CG(tmOff);
            CG(prOff);
            ok=true;
        }
        if (prefix == "BME2802")
        {
            busId = 1;
            addr = 0x77;
            #undef CG
            #define CG(name) CFG_GET(name, BME2802)
            CG(prAct);
            CG(tmAct);
            CG(huAct);
            CG(tmSrc);
            CG(huSrc);
            CG(iid);
            CG(intv);
            CG(tmNam);
            CG(huNam);
            CG(prNam);
            CG(tmOff);
            CG(prOff);
            ok=true;
        }
        if (prefix == "BME2803")
        {
            busId = 2;
            addr = 0x76;
            #undef CG
            #define CG(name) CFG_GET(name, BME2803)
            CG(prAct);
            CG(tmAct);
            CG(huAct);
            CG(tmSrc);
            CG(huSrc);
            CG(iid);
            CG(intv);
            CG(tmNam);
            CG(huNam);
            CG(prNam);
            CG(tmOff);
            CG(prOff);
            ok=true;
        }
        if (prefix == "BME2804")
        {
            busId = 1;
            addr = 0x77;
            #undef CG
            #define CG(name) CFG_GET(name, BME2804)
            CG(prAct);
            CG(tmAct);
            CG(huAct);
            CG(tmSrc);
            CG(huSrc);
            CG(iid);
            CG(intv);
            CG(tmNam);
            CG(huNam);
            CG(prNam);
            CG(tmOff);
            CG(prOff);
            ok=true;
        }
        intv *= 1000;
    }
};

void registerBME280(GwApi *api,SensorList &sensors){
    GwLog *logger=api->getLogger();
    #if defined(GWBME280) || defined(GWBME2801)
        BME280Config *cfg=new BME280Config(api,"BME2801");
        sensors.add(api,cfg);
        LOG_DEBUG(GwLog::LOG,"%s configured %d",cfg->prefix.c_str(),(int)cfg->ok);
    #endif
    #if defined(GWBME2802)
        BME280Config *cfg=new BME280Config(api,"BME2802");
        sensors.add(api,cfg);
    #endif
    #if defined(GWBME2803)
        BME280Config *cfg=new BME280Config(api,"BME2803");
        sensors.add(api,cfg);
    #endif
    #if defined(GWBME2804)
        BME280Config *cfg=new BME280Config(api,"BME2804");
        sensors.add(api,cfg);
    #endif
}
#else
void registerBME280(GwApi *api,SensorList &sensors){   
}

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
    #if defined(GWQMP6988) || defined(GWQMP69881)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69881");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWQMP69882)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69882");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWQMP69883)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69883");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWQMP69884)
    {
        QMP6988Config *scfg=new QMP6988Config(api,"QMP69884");
        LOG_DEBUG(GwLog::LOG,"%s configured",scfg->prefix.c_str());
        sensors.add(api,scfg);
    }
    #endif
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
