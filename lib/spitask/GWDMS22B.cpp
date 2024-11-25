/*
  (C) Andreas Vogel andreas@wellenvogel.de
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "GWDMS22B.h"
#include "GwApi.h"
#include "N2kMessages.h"
#include "GwHardware.h"


#define CHECK_BUS(BUS) \
    checkDef("missing config for " #BUS,GW ## BUS ## _CLK ,GW ## BUS ## _MISO);


class GWDMS22B;
static GwSensorConfigInitializerList<GWDMS22B> configs;


class GWDMS22B : public SSISensor{
    public:
    int zero=2047;
    bool invt=false;
    String zeroConfigName;
    GWDMS22B(GwApi *api,const String &prfx):SSISensor(api,prfx){}
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"DMS22B configured, prefix=%s, intv=%f, active=%d",prefix.c_str(),fintv,(int)act);
        return act;
    }
    virtual void measure(GwApi * api,BUSTYPE *bus, int counterId){
        GwLog *logger=api->getLogger();
        uint32_t value=0;
        esp_err_t res=readData(value);
        if (res != ESP_OK){
            LOG_DEBUG(GwLog::ERROR,"unable to measure %s: %d",prefix.c_str(),(int)res);
        }
        double resolved=(((int)value-zero)*360.0/mask);
        if (invt) resolved=-resolved;
        LOG_DEBUG(GwLog::DEBUG,"measure %s : %d, resolved: %f",prefix.c_str(),value,(float)resolved);
        tN2kMsg msg;
        SetN2kRudder(msg,DegToRad(resolved),iid);
        api->sendN2kMessage(msg);
        api->increment(counterId,prefix);
        api->setCalibrationValue(zeroConfigName,(double)value);
    }
    
    virtual void readConfig(GwConfigHandler *cfg){
        if (ok) return;
        configs.readConfig(this,cfg);
    }
};

#define ADD22B(PRFX,BUS) \
        {\
        CHECK_BUS(BUS); \
        GWDMS22B *dms=new GWDMS22B(api,#PRFX);\
        api->addSensor(dms,true); \
        }

void registerDMS22B(GwApi *api){
#ifdef GWDMS22B11
    ADD22B(DMS22B11,SPI1)
    #ifndef GWDMS22B11_CS
        #define GWDMS22B11_CS -1
    #endif
#else
    #define GWDMS22B11_CS -1
#endif

#ifdef GWDMS22B12
    ADD22B(DMS22B12,SPI1)
    #ifndef GWDMS22B12_CS
        #error "you need to define GWDMS22B12_CS"
    #endif
    #if GWDMS22B11_CS == -1
        #error "multiple devices on one SPI bus need chip select defines - GWDMS22B11_CS is unset"
    #endif
#else
    #define GWDMS22B12_CS -1
#endif


#ifdef GWDMS22B21
    ADD22B(DMS22B21,SPI2)
    #ifndef GWDMS22B21_CS
        #define GWDMS22B21_CS -1
    #endif
#else
    #define GWDMS22B21_CS -1
#endif

#ifdef GWDMS22B22
    ADD22B(DMS22B22,SPI2)
    #ifndef GWDMS22B22_CS
        #error "you need to define GWDMS22B22_CS"
    #endif
    #if GWDMS22B21_CS == -1
        #error "multiple devices on one SPI bus need chip select defines - GWDMS22B21_CS is unset"
    #endif
#else
    #define GWDMS22B22_CS -1
#endif
}


#define CFGDMS22B(s,PRFX,bus,csv) \
            CFG_SGET(s,act,PRFX); \
            CFG_SGET(s,iid,PRFX); \
            CFG_SGET(s,fintv,PRFX); \
            CFG_SGET(s,zero,PRFX); \
            s->zeroConfigName=GwConfigDefinitions::PRFX ## zero;\
            CFG_SGET(s,invt,PRFX); \
            s->bits=12; \
            s->clock=500000; \
            s->cs=csv; \
            s->busId=bus; \
            s->intv=1000*s->fintv; \
            s->ok=true;
    
#define SCDMS22B(prefix,bus) \
    GWSENSORDEF(configs,GWDMS22B,CFGDMS22B,prefix,GW ## bus ## _HOST,GW ## prefix ## _CS)

SCDMS22B(DMS22B11,SPI1);
SCDMS22B(DMS22B12,SPI1);
SCDMS22B(DMS22B21,SPI2);
SCDMS22B(DMS22B22,SPI2);
