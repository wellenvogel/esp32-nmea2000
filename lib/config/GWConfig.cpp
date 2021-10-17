#include "GWConfig.h"
#include <ArduinoJson.h>

#define B(v) (v?"true":"false")

bool isTrue(const char * value){
    return (strcasecmp(value,"true") == 0);
}

class DummyConfig : public GwConfigItem{
    public:
        DummyConfig():GwConfigItem("dummy",""){}
        virtual void fromString(const String v){
        };
        virtual void reset(){
        }
};

DummyConfig dummyConfig;
String GwConfigHandler::toString() const{
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

String GwConfigHandler::toJson() const{
    String rt;
    DynamicJsonDocument jdoc(50);
    for (int i=0;i<getNumConfig();i++){
        jdoc[configs[i]->getName()]=configs[i]->asString();
    }
    serializeJson(jdoc,rt);
    return rt;
}
GwConfigItem * GwConfigHandler::findConfig(const String name, bool dummy){
    for (int i=0;i<getNumConfig();i++){
        if (configs[i]->getName() == name) return configs[i];
    }
    if (!dummy) return NULL;
    return &dummyConfig;
}
GwConfigInterface * GwConfigHandler::getConfigItem(const String name, bool dummy) const{
    for (int i=0;i<getNumConfig();i++){
        if (configs[i]->getName() == name) return configs[i];
    }
    if (!dummy) return NULL;
    return &dummyConfig;
}
#define PREF_NAME "gwprefs"
GwConfigHandler::GwConfigHandler(GwLog *logger){
    this->logger=logger;
}
bool GwConfigHandler::loadConfig(){
    logger->logString("config load");
    prefs.begin(PREF_NAME,true);
    for (int i=0;i<getNumConfig();i++){
        String v=prefs.getString(configs[i]->getName().c_str(),configs[i]->getDefault());
    }
    prefs.end();
    return true;
}
bool GwConfigHandler::saveConfig(){
    prefs.begin(PREF_NAME,false);
    for (int i=0;i<getNumConfig();i++){
        prefs.putString(configs[i]->getName().c_str(),configs[i]->asString());
    }
    prefs.end();
    logger->logString("saved config");
    return true;
}
bool GwConfigHandler::updateValue(const char *name, const char * value){
    GwConfigItem *i=findConfig(name);
    if (i == NULL) return false;
    logger->logString("update config %s=>%s",name,value);
    i->fromString(value);
}
bool GwConfigHandler::reset(bool save){
    logger->logString("reset config");
    for (int i=0;i<getNumConfig();i++){
        configs[i]->reset();
    }
    if (!save) return true;
    return saveConfig();
}
String GwConfigHandler::getString(const String name){
    GwConfigItem *i=findConfig(name);
    if (!i) return String();
    return i->asString();
}
bool GwConfigHandler::getBool(const String name){
    GwConfigItem *i=findConfig(name);
    if (!i) return false;
    return i->asBoolean();
}