#include "GWConfig.h"
#include <ArduinoJson.h>
#include <string.h>

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
    int num=getNumConfig();
    DynamicJsonDocument jdoc(JSON_OBJECT_SIZE(num*2));
    for (int i=0;i<num;i++){
        jdoc[configs[i]->getName()]=configs[i]->asCString();
    }
    serializeJson(jdoc,rt);
    logger->logString("configJson: %s",rt.c_str());
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
GwConfigHandler::GwConfigHandler(GwLog *logger): GwConfigDefinitions(){
    this->logger=logger;
}
bool GwConfigHandler::loadConfig(){
    prefs.begin(PREF_NAME,true);
    for (int i=0;i<getNumConfig();i++){
        String v=prefs.getString(configs[i]->getName().c_str(),configs[i]->getDefault());
        configs[i]->fromString(v);
    }
    prefs.end();
    return true;
}
bool GwConfigHandler::saveConfig(){
    prefs.begin(PREF_NAME,false);
    for (int i=0;i<getNumConfig();i++){
        logger->logString("saving %s=%s",configs[i]->getName().c_str(),configs[i]->asCString());
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
    return true;
}
bool GwConfigHandler::updateValue(String name, String value){
    GwConfigItem *i=findConfig(name);
    if (i == NULL) return false;
    logger->logString("update config %s=>%s",name.c_str(),value.c_str());
    i->fromString(value);
    return true;
}
bool GwConfigHandler::reset(bool save){
    logger->logString("reset config");
    for (int i=0;i<getNumConfig();i++){
        configs[i]->reset();
    }
    if (!save) return true;
    return saveConfig();
}
String GwConfigHandler::getString(const String name, String defaultv) const{
    GwConfigInterface *i=getConfigItem(name,false);
    if (!i) return defaultv;
    return i->asString();
}
bool GwConfigHandler::getBool(const String name, bool defaultv) const{
    GwConfigInterface *i=getConfigItem(name,false);
    if (!i) return defaultv;
    return i->asBoolean();
}
int GwConfigHandler::getInt(const String name,int defaultv) const{
    GwConfigInterface *i=getConfigItem(name,false);
    if (!i) return defaultv;
    return i->asInt();
}

void GwNmeaFilter::parseFilter(){
    if (isReady) return;
    int found=0;
    int last=0;
    String data=config->asString();
    while ((found = data.indexOf(',',last)) >= 0){
        String tok=data.substring(last,found);
        if (tok != ""){
            if (tok.startsWith("^")) blacklist.push_back(tok);
            else whitelist.push_back(tok);
        }
        last=found+1;
    }
    if (last < data.length()){
        String tok=data.substring(last);
        if (tok != "" && tok != "^" ){
            if (tok.startsWith("^")) blacklist.push_back(tok.substring(1));
            else whitelist.push_back(tok);
        }

    }
    isReady=true;    
}

bool GwNmeaFilter::canPass(const char *buffer){
    size_t len=strlen(buffer);
    if (len < 5) return false; //invalid NMEA
    if (!isReady) parseFilter();
    bool hasWhitelist=false;
    for (auto it=blacklist.begin();it != blacklist.end();it++){
        if (buffer[0] == '$'){
            if ((strncmp(buffer,(*it).c_str(),1) == 0) &&
                (strncmp(buffer+3,(*it).c_str()+1,it->length()-1) == 0)
                ) return false;
        }
        else{
            if (strncmp(buffer,(*it).c_str(),it->length()) == 0) return false;
        }
    }
    for (auto it=whitelist.begin();it != whitelist.end();it++){
        hasWhitelist=true;
        if (buffer[0] == '$'){
            if ((strncmp(buffer,(*it).c_str(),1) == 0) &&
                (strncmp(buffer+3,(*it).c_str()+1,it->length()-1) == 0)
                ) return true;
        }
        else{
            if (strncmp(buffer,(*it).c_str(),it->length()) == 0) return true;
        }
    }
    return !hasWhitelist;
}