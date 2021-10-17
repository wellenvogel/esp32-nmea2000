#ifndef _GWCONFIG_H
#define _GWCONFIG_H
#include <Arduino.h>
#include <Preferences.h>
#include "GwLog.h"

class GwConfigInterface{
    public:
    virtual String asString() const=0;
    virtual const char * asCString() const =0;
    virtual bool asBoolean() const = 0;
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
        GwConfigItem(const String &name, bool initalValue):
            GwConfigItem(name,initalValue?String("true"):String("false")){};
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


class GwConfigHandler{
    private:
        Preferences prefs;
        GwLog *logger;
    public:
        public:
        const String sendUsb="sendUsb";
        const String receiveUsb="receiveUsb";
        const String wifiClient="wifiClient";
        const String wifiPass="wifiPass";
        const String wifiSSID="wifiSSID";
        GwConfigHandler(GwLog *logger);
        bool loadConfig();
        bool saveConfig();
        bool updateValue(const char *name, const char * value);
        bool updateValue(String name, String value);
        bool reset(bool save);
        String toString() const;
        String toJson() const;
        String getString(const String name);
        bool getBool(const String name);
        GwConfigItem * findConfig(const String name, bool dummy=false);
        GwConfigInterface * getConfigItem(const String name, bool dummy=false) const;
    private:
    GwConfigItem* configs[5]={
        new GwConfigItem(sendUsb,"true"),
        new GwConfigItem (receiveUsb,"false"),
        new GwConfigItem (wifiClient,"false"),
        new GwConfigItem (wifiSSID,""),
        new GwConfigItem (wifiPass,"")
    };  
    int getNumConfig() const{
        return 5;
    }  
};
#endif