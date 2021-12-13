#ifndef _GWCONFIGITEM_H
#define _GWCONFIGITEM_H
#include "WString.h"
#include <vector>

class GwConfigHandler;
class GwConfigInterface{
    private:
        String name;
        String initialValue;
        String value;
        bool secret=false;
    public:
        GwConfigInterface(const String &name, const String initialValue, bool secret=false){
            this->name=name;
            this->initialValue=initialValue;
            this->value=initialValue;
            this->secret=secret;
        }
        virtual String asString() const{
            return value;
        }
        virtual const char * asCString() const{
            return value.c_str();
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
        virtual bool isSecret() const{
            return secret;
        }
        bool changed() const{
            return value != initialValue;
        }
        String getDefault() const {
            return initialValue;
        }
        friend class GwConfigHandler;
};

class GwNmeaFilter{
    private:
        GwConfigInterface *config=NULL;
        bool isReady=false;
        bool ais=true;
        bool blacklist=true;
        std::vector<String> filter;
        void handleToken(String token, int index);
        void parseFilter();
    public:
        GwNmeaFilter(GwConfigInterface *config){
            this->config=config;
            isReady=false;
        }
        bool canPass(const char *buffer);
        String toString();    
};


#endif