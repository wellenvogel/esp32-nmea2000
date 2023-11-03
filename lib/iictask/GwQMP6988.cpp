#include "GwQMP6988.h"
#ifdef _GWQMP6988
#define PRFX1 "QMP698811"
#define PRFX2 "QMP698812"
#define PRFX3 "QMP698821"
#define PRFX4 "QMP698822"
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
            if (prefix == PRFX1){
                busId=1;
                addr=86;
                #undef CG
                #define CG(name) CFG_GET(name,QMP698811)
                CG(prNam);
                CG(iid);
                CG(prAct);
                CG(intv);
                CG(prOff);
                ok=true;
            }
            if (prefix == PRFX2){
                busId=1;
                addr=112;
                #undef CG
                #define CG(name) CFG_GET(name,QMP698812)
                CG(prNam);
                CG(iid);
                CG(prAct);
                CG(intv);
                CG(prOff);
                ok=true;
            }
            if (prefix == PRFX3){
                busId=2;
                addr=86;
                #undef CG
                #define CG(name) CFG_GET(name,QMP698821)
                CG(prNam);
                CG(iid);
                CG(prAct);
                CG(intv);
                CG(prOff);
                ok=true;
            }
            if (prefix == PRFX4){
                busId=2;
                addr=112;
                #undef CG
                #define CG(name) CFG_GET(name,QMP698822)
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
void registerQMP6988(GwApi *api,SensorList &sensors){
    GwLog *logger=api->getLogger();
    #if defined(GWQMP6988) || defined(GWQMP69881)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX1);
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWQMP69882)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX2);
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWQMP69883)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX3);
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWQMP69884)
    {
        QMP6988Config *scfg=new QMP6988Config(api,PRFX4);
        sensors.add(api,scfg);
    }
    #endif
}

#else
    void registerQMP6988(GwApi *api,SensorList &sensors){}
#endif