#include "GwBME280.h"
#ifdef _GWIIC
    #if defined(GWBME280) || defined(GWBME2801) || defined(GWBME2802)|| defined(GWBME2803)|| defined(GWBME2804)
        #define _GWBME280
    #else
        #undef _GWBME280
    #endif
#else
    #undef _GWBME280
    #undef GWBME280
    #undef GWBME2801
    #undef GWBME2802
    #undef GWBME2803
    #undef GWBME2804
#endif
#ifdef _GWBME280
    #include <Adafruit_BME280.h>
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

