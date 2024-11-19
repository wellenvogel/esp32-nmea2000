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
#define TYPE "BME280"
#define PRFX1 TYPE "11"
#define PRFX2 TYPE "12"
#define PRFX3 TYPE "21"
#define PRFX4 TYPE "22"
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
    BME280Config(GwApi * api, const String &prfx):SensorBase(TYPE,api,prfx){
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
    #define CFG280(prefix) \
        CFG_GET(prAct,prefix); \
        CFG_GET(tmAct,prefix);\
        CFG_GET(huAct,prefix);\
        CFG_GET(tmSrc,prefix);\
        CFG_GET(huSrc,prefix);\
        CFG_GET(iid,prefix);\
        CFG_GET(intv,prefix);\
        CFG_GET(tmNam,prefix);\
        CFG_GET(huNam,prefix);\
        CFG_GET(prNam,prefix);\
        CFG_GET(tmOff,prefix);\
        CFG_GET(prOff,prefix);

    virtual void readConfig(GwConfigHandler *cfg) override
    {
        if (ok) return;
        if (prefix == PRFX1)
        {
            busId = 1;
            addr = 0x76;
            CFG280(BME28011);
            ok=true;
        }
        if (prefix == PRFX2)
        {
            busId = 1;
            addr = 0x77;
            CFG280(BME28012);
            ok=true;
        }
        if (prefix == PRFX3)
        {
            busId = 2;
            addr = 0x76;
            CFG280(BME28021);
            ok=true;
        }
        if (prefix == PRFX4)
        {
            busId = 2;
            addr = 0x77;
            CFG280(BME28022);
            ok=true;
        }
        intv *= 1000;
    }
};

static SensorBase::Creator creator([](GwApi *api, const String &prfx){
    return new BME280Config(api,prfx);
});
SensorBase::Creator registerBME280(GwApi *api){
    #if defined(GWBME280) || defined(GWBME28011)
    {
        api->addSensor(creator(api,PRFX1));
        CHECK_IIC1();
        #pragma message "GWBME28011 defined"
    }
    #endif
    #if defined(GWBME28012)
    {
        api->addSensor(creator(api,PRFX2));
        CHECK_IIC1();
        #pragma message "GWBME28012 defined"
    }
    #endif
    #if defined(GWBME28021)
    {
        api->addSensor(creator(api,PRFX3));
        CHECK_IIC2();
        #pragma message "GWBME28021 defined"
    }
    #endif
    #if defined(GWBME28022)
    {
        api->addSensor(creator(api,PRFX4));
        CHECK_IIC1();
        #pragma message "GWBME28022 defined"
    }
    #endif
    return creator;
}
#else
SensorBase::Creator registerBME280(GwApi *api){ 
    return SensorBase::Creator(); 
}

#endif

