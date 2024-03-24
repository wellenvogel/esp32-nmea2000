#define _IIC_GROOVE_LIST
#include "GwQMP6988.h"
#ifdef _GWQMP6988
#define TYPE "QMP6988"
#define PRFX1 TYPE "11"
#define PRFX2 TYPE "12"
#define PRFX3 TYPE "21"
#define PRFX4 TYPE "22"
class QMP6988Config : public IICSensorBase{
    public:
        String prNam="Pressure";
        bool prAct=true;
        tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
        float prOff=0;
        QMP6988 *device=nullptr;
        QMP6988Config(GwApi* api,const String &prefix):SensorBase(TYPE,api,prefix){}
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
        #define CFG6988(prefix)\
            CFG_GET(prNam,prefix); \
            CFG_GET(iid,prefix); \
            CFG_GET(prAct,prefix); \
            CFG_GET(intv,prefix); \
            CFG_GET(prOff,prefix);
        
        virtual void readConfig(GwConfigHandler *cfg){
            if (ok) return;
            if (prefix == PRFX1){
                busId=1;
                addr=86;
                CFG6988(QMP698811);
                ok=true;
            }
            if (prefix == PRFX2){
                busId=1;
                addr=112;
                CFG6988(QMP698812);
                ok=true;
            }
            if (prefix == PRFX3){
                busId=2;
                addr=86;
                CFG6988(QMP698821);
                ok=true;
            }
            if (prefix == PRFX4){
                busId=2;
                addr=112;
                CFG6988(QMP698822);
                ok=true;
            }
            intv*=1000;

        }
};
static IICSensorBase::Creator creator=[](GwApi *api,const String &prfx){
    return new QMP6988Config(api,prfx);
};
IICSensorBase::Creator registerQMP6988(GwApi *api,IICSensorList &sensors){
    GwLog *logger=api->getLogger();
    #if defined(GWQMP6988) || defined(GWQMP698811)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX1);
        sensors.add(api,scfg);
        CHECK_IIC1();
        #pragma message "GWQMP698811 defined"
    }
    #endif
    #if defined(GWQMP698812)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX2);
        sensors.add(api,scfg);
        CHECK_IIC1();
        #pragma message "GWQMP698812 defined"
    }
    #endif
    #if defined(GWQMP698821)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX3);
        sensors.add(api,scfg);
        CHECK_IIC2();
        #pragma message "GWQMP698821 defined"
    }
    #endif
    #if defined(GWQMP698822)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX4);
        sensors.add(api,scfg);
        CHECK_IIC2();
        #pragma message "GWQMP698822 defined"
    }
    #endif
    return creator;
}

#else
    IICSensorBase::Creator registerQMP6988(GwApi *api,IICSensorList &sensors){
        return IICSensorBase::Creator();
    }
#endif