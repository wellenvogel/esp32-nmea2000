#ifndef _GWCONFIGITEM_H
#define _GWCONFIGITEM_H
#include "WString.h"
#include <vector>

class GwConfigHandler;
class GwConfigInterface{
    public:
        typedef enum {
            NORMAL=0,
            HIDDEN=1,
            READONLY=2
        } ConfigType;
    private:
        String name;
        const char * initialValue;
        String value;
        bool secret=false;
        ConfigType type=NORMAL;
    public:
        GwConfigInterface(const String &name, const char * initialValue, bool secret=false,ConfigType type=NORMAL){
            this->name=name;
            this->initialValue=initialValue;
            this->value=initialValue;
            this->secret=secret;
            this->type=type;
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
        ConfigType getType() const {
            return type;
        }
        friend class GwConfigHandler;
};

class GwNmeaFilter{
    private:
        String config;
        bool isReady=false;
        bool ais=true;
        bool blacklist=true;
        std::vector<String> filter;
        void handleToken(String token, int index);
        void parseFilter();
    public:
        GwNmeaFilter(String config){
            this->config=config;
            isReady=false;
        }
        bool canPass(const char *buffer);
        String toString();    
};

#define __XSTR(x) __STR(x)
#define __STR(x) #x
#define __MSG(x) _Pragma (__STR(message (x)))
#endif