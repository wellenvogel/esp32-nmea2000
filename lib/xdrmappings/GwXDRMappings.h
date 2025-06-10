#ifndef _GWXDRMAPPINGS_H
#define _GWXDRMAPPINGS_H
#include "GwLog.h"
#include "GwBoatData.h"
#include <WString.h>
#include <vector>
#include <map>
#include <unordered_set>
//enum must match the defines in xdrconfig.json
typedef enum {
    XDRTEMP=0,
    XDRHUMIDITY=1,
    XDRPRESSURE=2,
    XDRTIME=3, //unused
    XDRFLUID=4,
    XDRDCTYPE=5, //unused
    XDRBATTYPE=6, //unused
    XDRBATCHEM=7, //unused
    XDRGEAR=8, //unused
    XDRBAT=9,
    XDRENGINE=10,
    XDRATTITUDE=11
} GwXDRCategory;
class GwXDRType{
    public:
    typedef enum{
        PRESS=0,
        PERCENT=1,
        VOLT=2,
        AMP=3,
        TEMP=4,
        HUMID=5,
        VOLPERCENT=6,
        VOLUME=7,
        FLOW=8,
        GENERIC=9,
        DISPLACEMENT=10,
        RPM=11,
        DISPLACEMENTD=12,
        UNKNOWN=99
    }TypeCode;
    typedef double (* convert)(double);
    TypeCode code;
    String xdrtype;
    String xdrunit;
    String boatDataUnit;
    convert tonmea=NULL;
    convert fromnmea=NULL;
    GwXDRType(TypeCode tc,String xdrtype,String xdrunit){
        this->code=tc;
        this->xdrtype=xdrtype;
        this->xdrunit=xdrunit;
        this->boatDataUnit=xdrunit;
        this->fromnmea=fromnmea;
        this->tonmea=tonmea;
    }
    GwXDRType(TypeCode tc,String xdrtype,String xdrunit,convert fromnmea,convert tonmea,String boatDataUnit=String()){
        this->code=tc;
        this->xdrtype=xdrtype;
        this->xdrunit=xdrunit;
        this->fromnmea=fromnmea;
        this->tonmea=tonmea;
        this->boatDataUnit=boatDataUnit.isEmpty()?xdrunit:boatDataUnit;
    }
};
class GwXDRTypeMapping{
    public:
        GwXDRCategory category;
        int fieldIndex;
        GwXDRType::TypeCode type;
        GwXDRTypeMapping(int category,
            int fieldIndex,
            int type){
                this->category=(GwXDRCategory) category;
                this->type=(GwXDRType::TypeCode)type;
                this->fieldIndex=fieldIndex;
            }
};

class GwXDRMappingDef{
    public:
    typedef enum{
        IS_SINGLE=0,
        IS_IGNORE,
        IS_AUTO,
        IS_LAST
    } InstanceMode;
    typedef enum{
        M_DISABLED=0,
        M_BOTH=1,
        M_TO2K=2,
        M_FROM2K=3,
        M_LAST
    } Direction;
    String xdrName;
    GwXDRCategory category;
    int selector=-1;
    int field=0;
    InstanceMode instanceMode=IS_AUTO;
    int instanceId=-1;
    Direction direction=M_BOTH;
    GwXDRMappingDef(String xdrName,GwXDRCategory category,
        int selector, int field,InstanceMode istanceMode,int instance,
        Direction direction){
            this->xdrName=xdrName;
            this->category=category;
            this->selector=selector;
            this->field=field;
            this->instanceMode=instanceMode;
            this->instanceId=instance;
            this->direction=direction;
        };
    GwXDRMappingDef(){
        category=XDRTEMP;
    }
    //category,direction,selector,field,instanceMode,instance,name
    String toString() const;
    static GwXDRMappingDef *fromString(String s);
    //we allow 100 entities of code,selector and field nid
    static unsigned long n2kKey(GwXDRCategory category, int selector, int field)
    {
        long rt = ((int)category)&0xff;
        if (selector < 0)
            selector = 0;
        rt = (rt<<8) + (selector & 0xff);
        if (field < 0)
            field = 0;
        rt = (rt <<8) + (field & 0xff);
        return rt;
    }
    unsigned long n2kKey(){
        return n2kKey(category,selector,field);
    }
    static String n183key(String xdrName, String xdrType, String xdrUnit)
    {
        String rt = xdrName;
        rt += ",";
        rt += xdrType;
        rt += ",";
        rt += xdrUnit;
        return rt;
    }
    String getTransducerName(int instance) const;
    private:
    static bool handleToken(String tok,int index,GwXDRMappingDef *def);
};
class GwXDRMapping{
    public:
        GwXDRMappingDef *definition;
        GwXDRType *type;
        GwXDRMapping(GwXDRMappingDef *definition,GwXDRType *type){
            this->definition=definition;
            this->type=type;
        }
        typedef std::vector<GwXDRMapping*> MappingList;
        typedef std::map<String,MappingList> N138Map;
        typedef std::map<unsigned long,MappingList> N2KMap;
};
class GwXDRFoundMapping : public GwBoatItemNameProvider{
    public:
        class XdrEntry{
            public:
            String entry;
            String transducer;
        };
        const GwXDRMappingDef *definition=NULL;
        const GwXDRType *type=NULL;
        int instanceId=-1;
        bool empty=true;
        unsigned long timeout=0;
        GwXDRFoundMapping(const GwXDRMappingDef *definition,const GwXDRType *type, unsigned long timeout){
            this->definition=definition;
            this->type=type;
            this->timeout=timeout;
            empty=false;
        }
        GwXDRFoundMapping(GwXDRMapping* mapping,unsigned long timeout,int instance){
            this->definition=mapping->definition;
            this->type=mapping->type;
            this->instanceId=instance;
            this->timeout=timeout;
            empty=false;
        }
        GwXDRFoundMapping(){}
        virtual String getTransducerName(){
            return definition->getTransducerName(instanceId);
        }
        double valueFromXdr(double value){
            if (type->fromnmea) return (*(type->fromnmea))(value);
            return value;
        }
        XdrEntry buildXdrEntry(double value);
        //boat Data info
        virtual String getBoatItemName(){
            return String("xdr")+getTransducerName();
        };
        virtual String getBoatItemFormat(){
            return "formatXdr:"+type->xdrtype+":"+type->boatDataUnit; 
        };
        virtual ~GwXDRFoundMapping(){}
        virtual unsigned long getInvalidTime() override{
            return timeout;
        }
};

class GwXdrUnknownMapping : public GwXDRFoundMapping{
    String name;
    String unit;
    public:
    GwXdrUnknownMapping(const String &xdrName, const String &xdrUnit,const GwXDRType *type,unsigned long timeout):
        name(xdrName),unit(xdrUnit), GwXDRFoundMapping(nullptr,type,timeout){

    }
    virtual String getTransducerName(){
        return name;
    }
    virtual String getBoatItemFormat(){
        String rt=GwXDRFoundMapping::getBoatItemFormat();
        if (type->xdrunit.isEmpty()) rt+=unit;
        return rt;
    }
};

//the class GwXDRMappings is not intended to be deleted
//the deletion will leave memory leaks!
class GwConfigHandler;
class GwXDRMappings{
    static const int MAX_UNKNOWN=200;
    static const int ESIZE=13;
    private:
     GwLog *logger;
     GwConfigHandler *config;
     GwXDRMapping::N138Map n183Map;
     GwXDRMapping::N2KMap n2kMap;
     std::unordered_set<unsigned long> unknown;
     char *unknowAsString=NULL;
     GwXDRFoundMapping selectMapping(GwXDRMapping::MappingList *list,int instance,const char * key);
     bool addUnknown(GwXDRCategory category,int selector,int field=0,int instance=-1);
     bool addMapping(GwXDRMappingDef *mapping);
    public:
        GwXDRMappings(GwLog *logger,GwConfigHandler *config);
        bool addFixedMapping(const GwXDRMappingDef &mapping);
        void begin();
        //get the mappings
        //the returned mapping will exactly contain one mapping def
        GwXDRFoundMapping getMapping(String xName,String xType,String xUnit);
        GwXDRFoundMapping getMapping(GwXDRCategory category,int selector,int field=0,int instance=-1);
        String getXdrEntry(String mapping, double value,int instance=0);
        const char * getUnMapped();
        const GwXDRType * findType(const String &typeString, const String &unitString) const;

};

#endif