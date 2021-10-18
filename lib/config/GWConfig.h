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


class GwConfigHandler{
    private:
        Preferences prefs;
        GwLog *logger;
    public:
        public:
        const String sendUsb=F("sendUsb");
        const String receiveUsb=F("receiveUsb");
        const String wifiClient=F("wifiClient");
        const String wifiPass=F("wifiPass");
        const String wifiSSID=F("wifiSSID");
        const String serverPort=F("serverPort");
        const String maxClients=F("maxClients");
        const String sendTCP=F("sendTCP");
        const String sendSeasmart=F("sendSeasmart");
        GwConfigHandler(GwLog *logger);
        bool loadConfig();
        bool saveConfig();
        bool updateValue(const char *name, const char * value);
        bool updateValue(String name, String value);
        bool reset(bool save);
        String toString() const;
        String toJson() const;
        String getString(const String name) const;
        bool getBool(const String name) const ;
        int getInt(const String name) const;
        GwConfigItem * findConfig(const String name, bool dummy=false);
        GwConfigInterface * getConfigItem(const String name, bool dummy=false) const;
    private:
    GwConfigItem* configs[9]={
        new GwConfigItem(sendUsb,"true"),
        new GwConfigItem (receiveUsb,"false"),
        new GwConfigItem (wifiClient,"false"),
        new GwConfigItem (wifiSSID,""),
        new GwConfigItem (wifiPass,""),
        new GwConfigItem (serverPort,"2222"),
        new GwConfigItem (maxClients, "10"),
        new GwConfigItem (sendTCP,"true"),
        new GwConfigItem (sendSeasmart,"false")
    };  
    int getNumConfig() const{
        return 9;
    }  
};
#endif