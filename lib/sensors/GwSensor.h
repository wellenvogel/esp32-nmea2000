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
#include "GwConfigItem.h"
#include <Arduino.h>
#include <memory>
#include <vector>
class GwApi;
class GwConfigHandler;
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
    long intv=0;
    bool ok=false;
    SensorBase(BusType bt,GwApi *api,const String &prfx)
        :busType(bt),prefix(prfx){
    }
    using Creator=std::function<SensorBase *(GwApi *api,const String &prfx)>;
    virtual bool isActive(){return false;};
    virtual bool initDevice(GwApi *api,void *bus){return false;};
    virtual bool preinit(GwApi * api){return false;}
    virtual void measure(GwApi * api,void *bus, int counterId){};
    virtual ~SensorBase(){}
    virtual void readConfig(GwConfigHandler *cfg)=0;

};
template<typename BUS,SensorBase::BusType bt,typename Sensor>
class SensorTemplate : public SensorBase{
    public:
    using ConfigReader=std::function<bool(Sensor *,GwConfigHandler *)>;
    protected:
    ConfigReader configReader;
    public:
    SensorTemplate(GwApi *api,const String &prfx)
        :SensorBase(bt,api,prfx){}
    virtual bool initDevice(GwApi *api,BUS *bus){return false;};
    virtual bool initDevice(GwApi *api,void *bus){
        if (busType != bt) return false;
        return initDevice(api,(BUS*)bus);
    }
    virtual void measure(GwApi * api,void *bus, int counterId){
        if (busType != bt) return;
        measure(api,(BUS*)bus,counterId);
    };
    virtual void setConfigReader(ConfigReader r){
        configReader=r;
    }
    protected:
    bool configFromReader(Sensor *s, GwConfigHandler *cfg){
        if (configReader){
            return configReader(s,cfg);
        }
        return false;
    }
};


class SensorList : public std::vector<SensorBase::Ptr>{
    public:
    void add(SensorBase::Ptr sensor);
    using std::vector<SensorBase::Ptr>::vector;
};

/**
 * helper classes for a simplified sensor configuration
 * by creating a list of type GwSensorConfigInitializerList<SensorClass> we can populate
 * it by static instances of GwSensorConfigInitializer (using GWSENSORCONFIG ) that each has
 * a function that populates the sensor config from the config data. 
 * For using sensors this is not really necessary - but it can simplify the code for a sensor
 * if you want to support a couple of instances on different buses. 
 * By using this approach you still can write functions using the CFG_SGET macros that will check
 * your config definitions at compile time.
 * 
*/
template <typename T>
class GwSensorConfig{
    public:
    using ReadConfig=std::function<void(T*,GwConfigHandler*)>;
    ReadConfig configReader;
    String prefix;
    GwSensorConfig(const String &prfx,ReadConfig f):prefix(prfx),configReader(f){
    }
    bool readConfig(T* s,GwConfigHandler *cfg){
        if (s == nullptr) return false;
        configReader(s,cfg);
        return s->ok;
    }
};

template<typename T>
class GwSensorConfigInitializer : public GwInitializer<GwSensorConfig<T>>{
    public:
    using GwInitializer<GwSensorConfig<T>>::GwInitializer;
};
template<typename T>
class GwSensorConfigInitializerList : public GwInitializer<GwSensorConfig<T>>::List{
    public:
    using GwInitializer<GwSensorConfig<T>>::List::List;
    bool readConfig(T *s,GwConfigHandler *cfg){
        for (auto &&scfg:*this){
            if (scfg.readConfig(s,cfg)) return true;
        }
        return false;
    }
    bool knowsPrefix(const String &prefix){
        for (auto &&scfg:*this){
            if (scfg.prefix == prefix) return true;
        }
        return false;
    }
};

#define CFG_SGET(s, name, prefix) \
    cfg->getValue(s->name, GwConfigDefinitions::prefix##name)

#define GWSENSORCONFIG(list,type,prefix,initFunction) \
    GwSensorConfigInitializer<type> __init ## type ## prefix(list,GwSensorConfig<type>(#prefix,initFunction));



#define CFG_GET(name,prefix) \
    cfg->getValue(name, GwConfigDefinitions::prefix ## name)

    
#endif