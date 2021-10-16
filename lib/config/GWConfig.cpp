#include "GWConfig.h"
#include <ArduinoJson.h>

#define B(v) (v?"true":"false")

bool isTrue(const char * value){
    return (strcasecmp(value,"true") == 0);
}

class DummyConfig : public ConfigItem{
    public:
        DummyConfig():ConfigItem("dummy",""){}
        virtual void fromString(const String v){
        };
        virtual void reset(){
        }
};

DummyConfig dummyConfig;
String ConfigHandler::toString() const{
        String rt;
        rt+="Config: ";
        for (int i=0;i<getNumConfig();i++){
            rt+=configs[i]->getName();
            rt+="=";
            rt+=configs[i]->asString();
            rt+=", ";
        }
        return rt;   
    }

String ConfigHandler::toJson() const{
    String rt;
    DynamicJsonDocument jdoc(50);
    for (int i=0;i<getNumConfig();i++){
        jdoc[configs[i]->getName()]=configs[i]->asString();
    }
    serializeJson(jdoc,rt);
    return rt;
}
ConfigItem * ConfigHandler::findConfig(const String name, bool dummy){
    for (int i=0;i<getNumConfig();i++){
        if (configs[i]->getName() == name) return configs[i];
    }
    if (!dummy) return NULL;
    return &dummyConfig;
}

#define PREF_NAME "gwprefs"
ConfigHandler::ConfigHandler(){

}
bool ConfigHandler::loadConfig(){
    prefs.begin(PREF_NAME,true);
    for (int i=0;i<getNumConfig();i++){
        String v=prefs.getString(configs[i]->getName().c_str(),configs[i]->getDefault());
    }
    prefs.end();
    return true;
}
bool ConfigHandler::saveConfig(){
    prefs.begin(PREF_NAME,false);
    for (int i=0;i<getNumConfig();i++){
        prefs.putString(configs[i]->getName().c_str(),configs[i]->asString());
    }
    prefs.end();
    return true;
}
bool ConfigHandler::updateValue(const char *name, const char * value){
    return false;
}
bool ConfigHandler::reset(bool save){
    for (int i=0;i<getNumConfig();i++){
        configs[i]->reset();
    }
    if (!save) return true;
    return saveConfig();
}
String ConfigHandler::getString(const String name){
    ConfigItem *i=findConfig(name);
    if (!i) return String();
    return i->asString();
}
bool ConfigHandler::getBool(const String name){
    ConfigItem *i=findConfig(name);
    if (!i) return false;
    return i->asBoolean();
}