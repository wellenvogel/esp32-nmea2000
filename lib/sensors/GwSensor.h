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
#ifndef _GWSENSORS_H
#define _GWSENSORS_H
#include "GwApi.h"
#include "GwLog.h"
template<typename BUS>
class SensorBase{
    public:
    int busId=0;
    int iid=99; //N2K instanceId
    int addr=-1;
    String prefix;
    long intv=0;
    bool ok=false;
    virtual void readConfig(GwConfigHandler *cfg)=0;
    SensorBase(GwApi *api,const String &prfx):prefix(prfx){
    }
    virtual bool isActive(){return false;};
    virtual bool initDevice(GwApi *api,BUS *wire){return false;};
    virtual bool preinit(GwApi * api){return false;}
    virtual void measure(GwApi * api,BUS *wire, int counterId){};
    virtual ~SensorBase(){}
};

template<typename BUS>
class SensorList : public std::vector<SensorBase<BUS>*>{
    public:
    void add(GwApi *api, SensorBase<BUS> *sensor){
        sensor->readConfig(api->getConfig());
        api->getLogger()->logDebug(GwLog::LOG,"configured sensor %s, status %d",sensor->prefix.c_str(),(int)sensor->ok);
        this->push_back(sensor);
    }
    using std::vector<SensorBase<BUS>*>::vector;
};

#define CFG_GET(name,prefix) \
    cfg->getValue(name, GwConfigDefinitions::prefix ## name)
    
#endif