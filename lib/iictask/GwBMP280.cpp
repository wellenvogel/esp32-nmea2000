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
#define TYPE "BMP280"
#define PRFX1 TYPE "11"
#define PRFX2 TYPE "12"
#define PRFX3 TYPE "21"
#define PRFX4 TYPE "22"
class BMP280Config : public IICSensorBase{
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
    BMP280Config(GwApi * api, const String &prfx):IICSensorBase(TYPE,api,prfx){
    }
    virtual bool isActive(){return prAct||tmAct;}
    virtual bool initDevice(GwApi *api,TwoWire *wire){
        GwLog *logger=api->getLogger();  
        device= new Adafruit_BMP280(wire);
        if (! device->begin(addr)){
            LOG_DEBUG(GwLog::ERROR,"unable to initialize %s at %d",prefix.c_str(),addr);
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
    #define CFGBMP280(prefix) \
        CFG_GET(prAct,prefix); \
        CFG_GET(tmAct,prefix);\
        CFG_GET(tmSrc,prefix);\
        CFG_GET(iid,prefix);\
        CFG_GET(intv,prefix);\
        CFG_GET(tmNam,prefix);\
        CFG_GET(prNam,prefix);\
        CFG_GET(tmOff,prefix);\
        CFG_GET(prOff,prefix);

    virtual void readConfig(GwConfigHandler *cfg) override
    {
        if (prefix == PRFX1)
        {
            busId = 1;
            addr = 0x76;
            CFGBMP280(BMP28011);
            ok=true;
        }
        if (prefix == PRFX2)
        {
            busId = 1;
            addr = 0x77;
            CFGBMP280(BMP28012);
            ok=true;
        }
        if (prefix == PRFX3)
        {
            busId = 2;
            addr = 0x76;
            CFGBMP280(BMP28021);
            ok=true;
        }
        if (prefix == PRFX4)
        {
            busId = 2;
            addr = 0x77;
            CFGBMP280(BMP28022);
            ok=true;
        }
        intv *= 1000;
    }
};

static IICSensorBase::Creator creator([](GwApi *api, const String &prfx){
    return new BMP280Config(api,prfx);
});
IICSensorBase::Creator registerBMP280(GwApi *api,IICSensorList &sensors){
    #if defined(GWBMP280) || defined(GWBMP28011)
    {   
        auto *cfg=creator(api,PRFX1);
        //BMP280Config *cfg=new BMP280Config(api,PRFX1);
        sensors.add(api,cfg);
        CHECK_IIC1();
        #pragma message "GWBMP28011 defined"
    }
    #endif
    #if defined(GWBMP28012)
    {
        auto *cfg=creator(api,PRFX2);
        //BMP280Config *cfg=new BMP280Config(api,PRFX2);
        sensors.add(api,cfg);
        CHECK_IIC1();
        #pragma message "GWBMP28012 defined"
    }
    #endif
    #if defined(GWBMP28021)
    {
        auto *cfg=creator(api,PRFX3);
        //BMP280Config *cfg=new BMP280Config(api,PRFX3);
        sensors.add(api,cfg);
        CHECK_IIC2();
        #pragma message "GWBMP28021 defined"
    }
    #endif
    #if defined(GWBMP28022)
    {
        auto *cfg=creator(api,PRFX4);
        //BMP280Config *cfg=new BMP280Config(api,PRFX4);
        sensors.add(api,cfg);
        CHECK_IIC1();
        #pragma message "GWBMP28022 defined"
    }
    #endif
    return creator;
}
#else
IICSensorBase::Creator registerBMP280(GwApi *api,IICSensorList &sensors){
   return IICSensorBase::Creator();
}

#endif


