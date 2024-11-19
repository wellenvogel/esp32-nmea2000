#include "GwBMP280.h"
#ifdef _GWIIC
    #if defined(GWBMP280) || defined(GWBMP28011) || defined(GWBMP28012)|| defined(GWBMP28021)|| defined(GWBMP28022)
        #define _GWBMP280
    #endif
#else
    #undef _GWBMP280
    #undef GWBMP280
    #undef GWBMP28011
    #undef GWBMP28012
    #undef GWBMP28021
    #undef GWBMP28022
#endif
#ifdef _GWBMP280
    #include <Adafruit_BMP280.h>
#endif
#ifdef _GWBMP280


/**
 * we need a forward declaration here as the config list has to go before the
 * class implementation
 */

class BMP280Config;
GwSensorConfigInitializerList<BMP280Config> configs;

class BMP280Config : public IICSensorBase<BMP280Config>{
    public:
    bool prAct=true;
    bool tmAct=true;
    tN2kTempSource tmSrc=tN2kTempSource::N2kts_InsideTemperature;
    tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
    tN2kHumiditySource huSrc=tN2kHumiditySource::N2khs_Undef;
    String tmNam="Temperature";
    String prNam="Pressure";
    float tmOff=0;
    float prOff=0;
    Adafruit_BMP280 *device=nullptr;
    uint32_t sensorId=-1;
    using IICSensorBase<BMP280Config>::IICSensorBase;
    virtual bool isActive(){return prAct||tmAct;}
    virtual bool initDevice(GwApi *api,TwoWire *wire){
        GwLog *logger=api->getLogger();  
        device= new Adafruit_BMP280(wire);
        if (! device->begin(addr)){
            LOG_DEBUG(GwLog::ERROR,"unable to initialize %s at 0x%x",prefix.c_str(),addr);
            delete device;
            device=nullptr;
            return false;
        }
        sensorId=device->sensorID();
        LOG_DEBUG(GwLog::LOG, "initialized %s at %d, sensorId 0x%x", prefix.c_str(), addr, sensorId);
        return (sensorId == 0x56 || sensorId == 0x57 || sensorId == 0x58)?true:false;
    }
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"%s configured",prefix.c_str());
        api->addCapability(prefix,"true");
        addPressureXdr(api,*this);
        addTempXdr(api,*this);
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
        if (tmAct || prAct )
        {
            sendN2kEnvironmentalParameters(api, *this, temperature, humidity, computed,counterId);
        }
    }
    
    virtual void readConfig(GwConfigHandler *cfg) override
    {
        configs.readConfig(this,cfg);
    }
};


static SensorBase::Creator creator([](GwApi *api, const String &prfx)->BMP280Config*{
    if (! configs.knowsPrefix(prfx)){
        return nullptr;
    }
    return new BMP280Config(api,prfx);
});
SensorBase::Creator registerBMP280(GwApi *api){
    #if defined(GWBMP280) || defined(GWBMP28011)
    {   
        api->addSensor(creator(api,"BMP28011"));
        CHECK_IIC1();
        #pragma message "GWBMP28011 defined"
    }
    #endif
    #if defined(GWBMP28012)
    {
        api->addSensor(creator(api,"BMP28012"));
        CHECK_IIC1();
        #pragma message "GWBMP28012 defined"
    }
    #endif
    #if defined(GWBMP28021)
    {
        api->addSensor(creator(api,"BMP28021"));
        CHECK_IIC2();
        #pragma message "GWBMP28021 defined"
    }
    #endif
    #if defined(GWBMP28022)
    {
        api->addSensor(creator(api,"BMP28022"));
        CHECK_IIC1();
        #pragma message "GWBMP28022 defined"
    }
    #endif
    return creator;
}

/**
 * a define for the readConfig function
 * we use a define here as we want to be able to check the config
 * definitions at compile time
*/
#define CFGBMP280P(s, prefix, bus, baddr) \
    CFG_SGET(s, prAct, prefix);           \
    CFG_SGET(s, tmAct, prefix);           \
    CFG_SGET(s, tmSrc, prefix);           \
    CFG_SGET(s, iid, prefix);             \
    CFG_SGET(s, intv, prefix);            \
    CFG_SGET(s, tmNam, prefix);           \
    CFG_SGET(s, prNam, prefix);           \
    CFG_SGET(s, tmOff, prefix);           \
    CFG_SGET(s, prOff, prefix);           \
    s->busId = bus;                       \
    s->addr = baddr;                      \
    s->ok = true;                         \
    s->intv*=1000;

/**
 * a config initializer for our sensor
*/
#define SCBMP280(list, prefix, bus, addr) \
    GWSENSORCONFIG(list, BMP280Config, prefix, [](BMP280Config *s, GwConfigHandler *cfg) { CFGBMP280P(s, prefix, bus, addr); });

/**
 * four possible sensor configs
*/
SCBMP280(configs, BMP28011, 1, 0x76);
SCBMP280(configs, BMP28012, 1, 0x77);
SCBMP280(configs, BMP28021, 2, 0x76);
SCBMP280(configs, BMP28022, 2, 0x77);

#else
SensorBase::Creator registerBMP280(GwApi *api){
   return SensorBase::Creator();
}

#endif


