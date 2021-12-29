#include "GwChannelConfig.h"

GwChannelConfig::GwChannelConfig(GwLog *logger,String name){
    this->logger = logger;
    this->name=name;
    this->countIn=new GwCounter<String>(String("count")+name+String("in"));
    this->countOut=new GwCounter<String>(String("count")+name+String("out"));
}
void GwChannelConfig::begin(
    bool enabled,
    bool nmeaOut,
    bool nmeaIn,
    String readFilter,
    String writeFilter,
    bool seaSmartOut,
    bool toN2k)
{
    this->enabled = enabled;
    this->NMEAout = nmeaOut;
    this->NMEAin = nmeaIn;
    this->readFilter=readFilter.isEmpty()?
        NULL:
        new GwNmeaFilter(readFilter);
    this->writeFilter=writeFilter.isEmpty()?
        NULL:
        new GwNmeaFilter(writeFilter);
    this->seaSmartOut=seaSmartOut;
    this->toN2k=toN2k;
}
void GwChannelConfig::updateCounter(const char *msg, bool out)
{
    char key[6];
    if (msg[0] == '$')
    {
        strncpy(key, &msg[3], 3);
        key[3] = 0;
    }
    else if (msg[0] == '!')
    {
        strncpy(key, &msg[1], 5);
        key[5] = 0;
    }
    else{
        return;
    }
    if (out){
        countOut->add(key);
    }
    else{
        countIn->add(key);
    }
}
bool GwChannelConfig::canSendOut(unsigned long pgn){
    if (! enabled) return false;
    if (! NMEAout) return false;
    countOut->add(String(pgn)); 
    return true; 
}
bool GwChannelConfig::canReceive(unsigned long pgn){
    if (!enabled) return false;
    if (!NMEAin) return false;
    countIn->add(String(pgn));
    return true;
}

bool GwChannelConfig::canSendOut(const char *buffer){
    if (! enabled) return false;
    if (! NMEAout) return false;
    if (writeFilter && ! writeFilter->canPass(buffer)) return false;
    updateCounter(buffer,true);
    return true;
}

bool GwChannelConfig::canReceive(const char *buffer){
    if (! enabled) return false;
    if (! NMEAin) return false;
    if (readFilter && ! readFilter->canPass(buffer)) return false;
    updateCounter(buffer,false);
    return true;
}

int GwChannelConfig::getJsonSize(){
    if (! enabled) return 0;
    int rt=2;
    if (countIn) rt+=countIn->getJsonSize();
    if (countOut) rt+=countOut->getJsonSize();
    return rt;
}
void GwChannelConfig::toJson(GwJsonDocument &doc){
    if (! enabled) return;
    if (countOut) countOut->toJson(doc);
    if (countIn) countIn->toJson(doc);
}
String GwChannelConfig::toString(){
    String rt="CH:"+name;
    rt+=enabled?"[ena]":"[dis]";
    rt+=NMEAin?"in,":"";
    rt+=NMEAout?"out,":"";
    rt+=String("RF:") + (readFilter?readFilter->toString():"[]");
    rt+=String("WF:") + (writeFilter?writeFilter->toString():"[]");
    rt+=String(",")+ (toN2k?"n2k":"");
    rt+=String(",")+ (seaSmartOut?"SM":"");
    return rt;
}
