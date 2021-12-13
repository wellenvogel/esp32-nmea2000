#include "GWConfig.h"
#include <ArduinoJson.h>
#include <string.h>

#define B(v) (v?"true":"false")

bool isTrue(const char * value){
    return (strcasecmp(value,"true") == 0);
}

class DummyConfig : public GwConfigInterface{
    public:
        DummyConfig():GwConfigInterface("dummy",""){}
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
        if (configs[i]->isSecret()){
            jdoc[configs[i]->getName()]="";    
        }
        else{
            jdoc[configs[i]->getName()]=configs[i]->asCString();
        }
    }
    serializeJson(jdoc,rt);
    logger->logString("configJson: %s",rt.c_str());
    return rt;
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
        configs[i]->value=v;
    }
    prefs.end();
    return true;
}
bool GwConfigHandler::saveConfig(){
    prefs.begin(PREF_NAME,false);
    for (int i=0;i<getNumConfig();i++){
        String val=configs[i]->asString();
        auto it=changedValues.find(configs[i]->getName());
        if (it != changedValues.end()){
            val=it->second;
        }
        logger->logString("saving %s=%s",configs[i]->getName().c_str(),val.c_str());
        prefs.putString(configs[i]->getName().c_str(),val);
    }
    prefs.end();
    logger->logString("saved config");
    return true;
}

bool GwConfigHandler::updateValue(String name, String value){
    GwConfigInterface *i=getConfigItem(name);
    if (i == NULL) return false;
    if (i->isSecret() && value.isEmpty()){
        LOG_DEBUG(GwLog::LOG,"skip empty password %s",name.c_str());
    }
    else{
        LOG_DEBUG(GwLog::LOG,"update config %s=>%s",name.c_str(),i->isSecret()?"***":value.c_str());
        changedValues[name]=value;
    }
    return true;
}
bool GwConfigHandler::reset(bool save){
    logger->logString("reset config");
    for (int i=0;i<getNumConfig();i++){
        changedValues[configs[i]->getName()]=configs[i]->getDefault();
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

void GwNmeaFilter::handleToken(String token, int index){
    switch(index){
        case 0:
            ais=token.toInt() != 0;
            break;
        case 1:
            blacklist=token.toInt() != 0;
            break;
        case 2:
            int found=0;
            int last=0;
            while ((found = token.indexOf(',',last)) >= 0){
                filter.push_back(token.substring(last,found));   
                last=found+1;
            }
            if (last < token.length()){
                filter.push_back(token.substring(last));   
            }
            break;
    }    
}
void GwNmeaFilter::parseFilter(){
    // "0:1:RMB,RMC"
    // 0: AIS off, 1:whitelist, list of sentences
    if (isReady) return;
    int found=0;
    int last=0;
    int index=0;
    String data=config->asString();
    while ((found = data.indexOf(':',last)) >= 0){
        String tok=data.substring(last,found);
        handleToken(tok,index);
        last=found+1;
        index++;
    }
    if (last < data.length()){
        String tok=data.substring(last);
        handleToken(tok,index);
    }
    isReady=true;    
}

bool GwNmeaFilter::canPass(const char *buffer){
    size_t len=strlen(buffer);
    if (len < 5) return false; //invalid NMEA
    if (!isReady) parseFilter();
    if (buffer[0] == '!') return ais;
    char sentence[4];
    strncpy(sentence,buffer+3,3);
    sentence[3]=0;
    for (auto it=filter.begin();it != filter.end();it++){
        if (strncmp(sentence,(*it).c_str(),3) == 0) return !blacklist;
    }
    //if we have a whitelist we return false
    //if nothing matches
    return blacklist; 
}
String GwNmeaFilter::toString(){
    parseFilter();
    String rt("NMEAFilter: ");
    rt+="ais: "+String(ais);
    rt+=", bl:"+String(blacklist);
    for (auto it=filter.begin();it != filter.end();it++){
        rt+=","+*it;
    }
    return rt;
}