#ifndef _GWXDRMAPPINGS_H
#define _GWXDRMAPPINGS_H
#include "GwLog.h"
#include "GWConfig.h"
#include <WString.h>
#include <vector>
//enum must match the defines in xdrconfig.json
typedef enum {
    XDRTEMP=0,
    XDRHUMIDITY=1,
    XDRPRSSURE=2,
    XDRTIME=3, //unused
    XDRFLUID=4,
    XDRDCTYPE=5, //unused
    XDRBATTYPE=6, //unused
    XDRBATCHEM=7, //unused
    XDRGEAR=8, //unused
    XDRBAT=9,
    XDRENGINE=10
} GwXDRCategory;
class GwXDRType{
    public:
    typedef enum{
        PRESS=0,
        PERCENT=1,
        VOLT=2,
        AMP=3
    }TypeCode;
    typedef double (* convert)(double);
    TypeCode code;
    String xdrtype;
    String xdrunit;
    convert tonmea=NULL;
    convert fromnmea=NULL;
    GwXDRType(TypeCode tc,String xdrtype,String xdrunit,convert fromnmea=NULL,convert tonmea=NULL){
        this->code=tc;
        this->xdrtype=xdrtype;
        this->xdrunit=xdrunit;
        this->fromnmea=fromnmea;
        this->tonmea=tonmea;
    }
};
class GwXDRTypeMapping{
    public:
        GwXDRCategory category;
        int fieldIndex;
        GwXDRType::TypeCode type;
        GwXDRTypeMapping(GwXDRCategory category,
            int fieldIndex,
            GwXDRType::TypeCode type){
                this->category=category;
                this->type=type;
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
    int field=-1;
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
    String toString();
    static GwXDRMappingDef *fromString(String s);    
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
        //we allow 100 entities of code,selector and field nid
        static long n2kKey(GwXDRType::TypeCode code,int selector,int field){
            long rt=(int)code;
            if (selector < 0) selector=0;
            rt=rt*100+selector;
            if (field < 0) field=0;
            rt=rt*100*field;
            return rt;
        }
        static String n183key(String xdrName,String xdrType,String xdrUnit){
            String rt=xdrName;
            rt+=",";
            rt+=xdrType;
            rt+=",";
            rt+=xdrUnit;
            return rt;
        }
};

class GwXDRMappings{
    private:
     GwLog *logger;
     GwConfigHandler *config;
    public:
        GwXDRMappings(GwLog *logger,GwConfigHandler *config);
        void begin();

};

#endif