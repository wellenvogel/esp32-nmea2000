#define _IIC_GROOVE_LIST
#include "GwQMP6988.h"
#ifdef _GWQMP6988

class QMP6988Config;
static GwSensorConfigInitializerList<QMP6988Config> configs;

class QMP6988Config : public IICSensorBase{
    public:
        String prNam="Pressure";
        bool prAct=true;
        tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
        float prOff=0;
        QMP6988 *device=nullptr;
        QMP6988Config(GwApi* api,const String &prefix):IICSensorBase(api,prefix){}
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
            if (ok) return;
            configs.readConfig(this,cfg);
        }
};
static SensorBase::Creator creator=[](GwApi *api,const String &prfx)-> SensorBase*{
    if (! configs.knowsPrefix(prfx)) return nullptr;
    return new QMP6988Config(api,prfx);
};
SensorBase::Creator registerQMP6988(GwApi *api){
    GwLog *logger=api->getLogger();
    #if defined(GWQMP6988) || defined(GWQMP698811)
    {
        api->addSensor(new QMP6988Config(api,"QMP698811"));
        CHECK_IIC1();
        #pragma message "GWQMP698811 defined"
    }
    #endif
    #if defined(GWQMP698812)
    {
        api->addSensor(new QMP6988Config(api,"QMP698812"));
        CHECK_IIC1();
        #pragma message "GWQMP698812 defined"
    }
    #endif
    #if defined(GWQMP698821)
    {
        api->addSensor(new QMP6988Config(api,"QMP698821"));
        CHECK_IIC2();
        #pragma message "GWQMP698821 defined"
    }
    #endif
    #if defined(GWQMP698822)
    {
        api->addSensor(new QMP6988Config(api,"QMP698822"));
        CHECK_IIC2();
        #pragma message "GWQMP698822 defined"
    }
    #endif
    return creator;
}

#define CFG6988(s,prefix,bus,baddr)\
            CFG_SGET(s,prNam,prefix); \
            CFG_SGET(s,iid,prefix); \
            CFG_SGET(s,prAct,prefix); \
            CFG_SGET(s,intv,prefix); \
            CFG_SGET(s,prOff,prefix); \
            s->busId = bus;                       \
            s->addr = baddr;                      \
            s->ok = true;                         \
            s->intv*=1000;


#define SC6988(prefix,bus,addr) \
    GWSENSORDEF(configs,QMP6988Config,CFG6988,prefix,bus,addr)

SC6988(QMP698811,1,86);
SC6988(QMP698812,1,112);
SC6988(QMP698821,2,86);
SC6988(QMP698822,2,112);

#else
    SensorBase::Creator registerQMP6988(GwApi *api){
        return SensorBase::Creator();
    }
#endif