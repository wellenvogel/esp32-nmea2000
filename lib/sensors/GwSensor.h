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
#include <memory>
class SensorBase{
    public:
    using BusType=enum{
        IIC=0,
        SPI=1,
        UNKNOWN=-1
    };
    using Ptr=std::shared_ptr<SensorBase>;
    BusType busType=BusType::UNKNOWN;
    int busId=0;
    int iid=99; //N2K instanceId
    int addr=-1;
    const String prefix;
    const String type;
    long intv=0;
    bool ok=false;
    virtual void readConfig(GwConfigHandler *cfg)=0;
    SensorBase(BusType bt,const String &tn,GwApi *api,const String &prfx)
        :busType(bt),type(tn),prefix(prfx){
    }
    using Creator=std::function<SensorBase *(GwApi *api,const String &prfx)>;
    virtual bool isActive(){return false;};
    virtual bool initDevice(GwApi *api,void *bus){return false;};
    virtual bool preinit(GwApi * api){return false;}
    virtual void measure(GwApi * api,void *bus, int counterId){};
    virtual ~SensorBase(){}
};
template<typename BUS,SensorBase::BusType bt>
class SensorTemplate : public SensorBase{
    public:
    SensorTemplate(const String &tn,GwApi *api,const String &prfx)
        :SensorBase(bt,tn,api,prfx){}
    virtual bool initDevice(GwApi *api,BUS *bus){return false;};
    virtual bool initDevice(GwApi *api,void *bus){
        if (busType != bt) return false;
        return initDevice(api,(BUS*)bus);
    }
    virtual void measure(GwApi * api,void *bus, int counterId){
        if (busType != bt) return;
        measure(api,(BUS*)bus,counterId);
    };
};


class SensorList : public std::vector<SensorBase::Ptr>{
    public:
    void add(GwApi *api, SensorBase::Ptr sensor){
        sensor->readConfig(api->getConfig());
        api->getLogger()->logDebug(GwLog::LOG,"configured sensor %s, status %d",sensor->prefix.c_str(),(int)sensor->ok);
        this->push_back(sensor);
    }
    using std::vector<SensorBase::Ptr>::vector;
};


#define CFG_GET(name,prefix) \
    cfg->getValue(name, GwConfigDefinitions::prefix ## name)
    
#endif