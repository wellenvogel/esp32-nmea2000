#ifndef _GWCONFIGITEM_H
#define _GWCONFIGITEM_H
#include "WString.h"
#include <vector>
class GwConfigInterface{
    public:
    virtual String asString() const=0;
    virtual const char * asCString() const =0;
    virtual bool asBoolean() const = 0;
    virtual int asInt() const = 0;
};
class GwConfigItem: public GwConfigInterface{
    private:
        String name;
        String initialValue;
        String value;
    public:
        GwConfigItem(const String &name, const String initialValue){
            this->name=name;
            this->initialValue=initialValue;
            this->value=initialValue;
        }
        virtual String asString() const{
            return value;
        }
        virtual const char * asCString() const{
            return value.c_str();
        };
        virtual void fromString(const String v){
            value=v;
        };
        virtual bool asBoolean() const{
            return strcasecmp(value.c_str(),"true") == 0;
        }
        virtual int asInt() const{
            return (int)value.toInt();
        }
        String getName() const{
            return name;
        }
        virtual void reset(){
            value=initialValue;
        }
        bool changed() const{
            return value != initialValue;
        }
        String getDefault() const {
            return initialValue;
        }
};

class GwNmeaFilter{
    private:
        GwConfigInterface *config=NULL;
        bool isReady=false;
        std::vector<String> whitelist;
        std::vector<String> blacklist;
        void parseFilter();
    public:
        GwNmeaFilter(GwConfigInterface *config){
            this->config=config;
            isReady=false;
        }
        bool canPass(const char *buffer);    
};


#endif