#define CFG_MESSAGES
#include <Preferences.h>
#include "GWConfig.h"
#include <ArduinoJson.h>
#include <string.h>
#include <MD5Builder.h>
using CfgInit=std::function<void(GwConfigHandler *)>;
static std::vector<CfgInit> cfgInits;
class CfgInitializer{
    public:
        CfgInitializer(CfgInit f){
            cfgInits.push_back(f);
        }
};
#define CFG_INIT(name,value,mode) \
    __MSG("config set " #name " " #value " " #mode); \
    static CfgInitializer _ ## name ## _init([](GwConfigHandler *cfg){ \
        cfg->setValue(GwConfigDefinitions::name,value,GwConfigInterface::mode); \
      });
#include "GwHardware.h"
#include "GwConfigDefImpl.h"

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
    LOG_DEBUG(GwLog::DEBUG,"configJson: %s",rt.c_str());
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
    saltBase=esp_random();
    configs=new GwConfigInterface*[getNumConfig()];
    populateConfigs(configs);
    for (auto &&init:cfgInits){
        init(this);
    }
    prefs=new Preferences();
}
GwConfigHandler::~GwConfigHandler(){
    delete prefs;
}
bool GwConfigHandler::loadConfig(){
    prefs->begin(PREF_NAME,true);
    for (int i=0;i<getNumConfig();i++){
        String v=prefs->getString(configs[i]->getName().c_str(),configs[i]->getDefault());
        configs[i]->value=v;
    }
    prefs->end();
    return true;
}

bool GwConfigHandler::updateValue(String name, String value){
    GwConfigInterface *i=getConfigItem(name);
    if (i == NULL) return false;
    if (i->isSecret() && value.isEmpty()){
        LOG_DEBUG(GwLog::LOG,"skip empty password %s",name.c_str());
    }
    else{
        if (i->asString() == value){
            return false;
        }
        LOG_DEBUG(GwLog::LOG,"update config %s=>%s",name.c_str(),i->isSecret()?"***":value.c_str());
        prefs->begin(PREF_NAME,false);
        prefs->putString(i->getName().c_str(),value);
        prefs->end();
    }
    return true;
}
bool GwConfigHandler::reset(){
    LOG_DEBUG(GwLog::LOG,"reset config");
    prefs->begin(PREF_NAME,false);
    for (int i=0;i<getNumConfig();i++){
        prefs->putString(configs[i]->getName().c_str(),configs[i]->getDefault());
    }
    prefs->end();
    return true;
}
String GwConfigHandler::getString(const String name, String defaultv) const{
    GwConfigInterface *i=getConfigItem(name,false);
    if (!i) return defaultv;
    return i->asString();
}
const char * GwConfigHandler::getCString(const String name, const char *defaultv) const{
    GwConfigInterface *i=getConfigItem(name,false);
    if (!i) return defaultv;
    return i->asCString();
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
void GwConfigHandler::stopChanges(){
    allowChanges=false;
}
bool GwConfigHandler::setValue(String name,String value, bool hide){
    return setValue(name,value,hide?GwConfigInterface::HIDDEN:GwConfigInterface::READONLY);
}
bool GwConfigHandler::setValue(String name, String value, GwConfigInterface::ConfigType type){    
    if (! allowChanges) return false;
    LOG_DEBUG(GwLog::LOG,"setValue for %s to %s, mode %d",
        name.c_str(),value.c_str(),(int)type);
    GwConfigInterface *i=getConfigItem(name,false);
    if (!i) return false;
    i->value=value;
    i->type=type;
    return true;
}

bool GwConfigHandler::checkPass(String hash){
    if (! getBool(useAdminPass)) return true;
    String pass=getString(adminPassword);
    unsigned long now=millis()/1000UL & ~0x7UL;
    MD5Builder builder;
    char buffer[2*sizeof(now)+1];
    for (int i=0;i< 5 ;i++){
        unsigned long base=saltBase+now;
        toHex(base,buffer,2*sizeof(now)+1);
        builder.begin();
        builder.add(buffer);
        builder.add(pass);
        builder.calculate();
        String md5=builder.toString();
        bool rt=hash == md5;
        logger->logDebug(GwLog::DEBUG,"checking pass %s, base=%ld, hash=%s, res=%d",
        hash.c_str(),base,md5.c_str(),(int)rt);
        if (rt) return true;
        now -= 8;
    }
    return false;
}
static char hv(uint8_t nibble){
  nibble=nibble&0xf;
  if (nibble < 10) return (char)('0'+nibble);
  return (char)('A'+nibble-10);
}
void GwConfigHandler::toHex(unsigned long v, char *buffer, size_t bsize)
{
    uint8_t *bp = (uint8_t *)&v;
    size_t i = 0;
    for (; i < sizeof(v) && (2 * i + 1) < bsize; i++)
    {
        buffer[2 * i] = hv((*bp) >> 4);
        buffer[2 * i + 1] = hv(*bp);
        bp++;
    }
    if ((2 * i) < bsize)
        buffer[2 * i] = 0;
}

std::vector<String> GwConfigHandler::getSpecial() const{
    std::vector<String> rt;
    rt.reserve(numSpecial());
    for (int i=0L;i<getNumConfig();i++){
        if (configs[i]->getType() != GwConfigInterface::NORMAL){
            rt.push_back(configs[i]->getName());
        };
    }
    return rt;
}
int GwConfigHandler::numSpecial() const{
    int rt=0;
    for (int i=0L;i<getNumConfig();i++){
        if (configs[i]->getType() != GwConfigInterface::NORMAL) rt++;
    }
    return rt;
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
    if (config.isEmpty()){
        isReady=true;
        return;
    }
    int found=0;
    int last=0;
    int index=0;
    while ((found = config.indexOf(':',last)) >= 0){
        String tok=config.substring(last,found);
        handleToken(tok,index);
        last=found+1;
        index++;
    }
    if (last < config.length()){
        String tok=config.substring(last);
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