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
#define PREFIX1 "DMS22B11"
class GWDMS22B : public SSISensor{
    uint32_t zero=2047;
    public:
    using SSISensor::SSISensor;
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"DMS22B configured, prefix=%s",prefix.c_str());
        api->addCapability(prefix,"true");
        return true;
    }
    virtual void measure(GwApi * api,BusType *bus, int counterId){
        GwLog *logger=api->getLogger();
        uint32_t value=0;
        esp_err_t res=readData(value);
        if (res != ESP_OK){
            LOG_DEBUG(GwLog::ERROR,"unable to measure %s: %d",prefix.c_str(),(int)res);
        }
        LOG_DEBUG(GwLog::LOG,"measure %s : %d",prefix.c_str(),value);
    }
    #define DMS22B(prefix)\
        CFG_GET(act,prefix); \
        CFG_GET(iid,prefix); \
        CFG_GET(intv,prefix); \
        CFG_GET(zero,prefix);
    virtual void readConfig(GwConfigHandler *cfg){
        if (prefix == PREFIX1){
            DMS22B(DMS22B11);
            busId=GWSPIHOST1;
            bits=12;
            clock=500000;
            #ifdef GWDMS22B11_CS
            cs=GWDMS22B11_CS;
            #else
            cs=-1;
            #endif
        }
        //TODO: other
    }
};

void registerDMS22B(GwApi *api,SpiSensorList &sensors){
    #ifdef GWDMS22B11
        GWDMS22B *dms=new GWDMS22B(api,PREFIX1);
        sensors.add(dms);
    #endif
}
