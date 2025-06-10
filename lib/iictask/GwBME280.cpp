#include "GwBME280.h"
#ifdef _GWIIC
    #if defined(GWBME280) || defined(GWBME28011) || defined(GWBME28012)|| defined(GWBME28021)|| defined(GWBME28022)
        #define _GWBME280
    #endif
#else
    #undef _GWBME280
    #undef GWBME280
    #undef GWBME28011
    #undef GWBME28012
    #undef GWBME28021
    #undef GWBME28022
#endif
#ifdef _GWBME280
    #include <Adafruit_BME280.h>
#endif
#ifdef _GWBME280

class BME280Config;
static GwSensorConfigInitializerList<BME280Config> configs;
class BME280Config : public IICSensorBase{
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
    BME280Config(GwApi * api, const String &prfx):IICSensorBase(api,prfx){
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
        float pressure = N2kDoubleNA;
        float temperature = N2kDoubleNA;
        float humidity = N2kDoubleNA;
        float computed = N2kDoubleNA;
        if (prAct)
        {
            pressure = device->readPressure();
            computed = pressure + prOff;
            LOG_DEBUG(GwLog::DEBUG, "%s measure %2.0fPa, computed %2.0fPa", prefix.c_str(), pressure, computed);
            sendN2kPressure(api, *this, computed, counterId);
        }
        if (tmAct)
        {
            temperature = device->readTemperature(); // offset is handled internally
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
        if (tmAct || prAct || (huAct && sensorId == 0x60))
        {
            sendN2kEnvironmentalParameters(api, *this, temperature, humidity, computed,counterId);
        }
    }
    

    virtual void readConfig(GwConfigHandler *cfg) override
    {
        if (ok) return;
        configs.readConfig(this,cfg);       
    }
};


static SensorBase::Creator creator([](GwApi *api, const String &prfx){
    return new BME280Config(api,prfx);
});
SensorBase::Creator registerBME280(GwApi *api){
#if defined(GWBME280) || defined(GWBME28011)
    {
        api->addSensor(creator(api,"BME28011"));
        CHECK_IIC1();
        #pragma message "GWBME28011 defined"
    }
#endif
#if defined(GWBME28012)
    {
        api->addSensor(creator(api,"BME28012"));
        CHECK_IIC1();
        #pragma message "GWBME28012 defined"
    }
#endif
#if defined(GWBME28021)
    {
        api->addSensor(creator(api,"BME28021"));
        CHECK_IIC2();
        #pragma message "GWBME28021 defined"
    }
#endif
#if defined(GWBME28022)
    {
        api->addSensor(creator(api,"BME28022"));
        CHECK_IIC1();
        #pragma message "GWBME28022 defined"
    }
#endif
    return creator;
}

#define CFG280(s, prefix, bus, baddr) \
    CFG_SGET(s, prAct, prefix);       \
    CFG_SGET(s, tmAct, prefix);       \
    CFG_SGET(s, huAct, prefix);       \
    CFG_SGET(s, tmSrc, prefix);       \
    CFG_SGET(s, huSrc, prefix);       \
    CFG_SGET(s, iid, prefix);         \
    CFG_SGET(s, intv, prefix);        \
    CFG_SGET(s, tmNam, prefix);       \
    CFG_SGET(s, huNam, prefix);       \
    CFG_SGET(s, prNam, prefix);       \
    CFG_SGET(s, tmOff, prefix);       \
    CFG_SGET(s, prOff, prefix);       \
    s->busId = bus;                   \
    s->addr = baddr;                  \
    s->ok = true;                     \
    s->intv *= 1000;

#define SCBME280(list, prefix, bus, addr) \
    GWSENSORCONFIG(list, BME280Config, prefix, [](BME280Config *s, GwConfigHandler *cfg) { CFG280(s, prefix, bus, addr); });

SCBME280(configs,BME28011,1,0x76);
SCBME280(configs,BME28012,1,0x77);
SCBME280(configs,BME28021,2,0x76);
SCBME280(configs,BME28022,2,0x77);

#else
SensorBase::Creator registerBME280(GwApi *api){ 
    return SensorBase::Creator(); 
}

#endif

