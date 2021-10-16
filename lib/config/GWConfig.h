#ifndef _GWCONFIG_H
#define _GWCONFIG_H
#include <Arduino.h>
#include <Preferences.h>

class ConfigItem{
    private:
        String name;
        String initialValue;
        String value;
    public:
        ConfigItem(const String &name, const String initialValue){
            this->name=name;
            this->initialValue=initialValue;
            this->value=initialValue;
        }
        ConfigItem(const String &name, bool initalValue):
            ConfigItem(name,initalValue?String("true"):String("false")){};
        String asString() const{
            return value;
        }
        virtual void fromString(const String v){
            value=v;
        };
        bool asBoolean() const{
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


class ConfigHandler{
    private:
        Preferences prefs;
    public:
        public:
        const String sendUsb="sendUsb";
        const String receiveUsb="receiveUsb";
        const String wifiClient="wifiClient";
        const String wifiPass="wifiPass";
        const String wifiSSID="wifiSSID";
        ConfigHandler();
        bool loadConfig();
        bool saveConfig();
        bool updateValue(const char *name, const char * value);
        bool reset(bool save);
        String toString() const;
        String toJson() const;
        String getString(const String name);
        bool getBool(const String name);
        ConfigItem * findConfig(const String name, bool dummy=false);
    private:
    ConfigItem* configs[5]={
        new ConfigItem(sendUsb,true),
        new ConfigItem (receiveUsb,false),
        new ConfigItem (wifiClient,false),
        new ConfigItem (wifiSSID,""),
        new ConfigItem (wifiPass,"")
    };  
    int getNumConfig() const{
        return sizeof(configs)/sizeof(ConfigItem*);
    }  
};
#endif