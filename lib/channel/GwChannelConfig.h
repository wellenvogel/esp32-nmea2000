#pragma once
#include "GwConfigItem.h"
#include "GwLog.h"
#include "GWConfig.h"
#include "GwCounter.h"
#include "GwJsonDocument.h"

class GwChannelConfig{
    bool enabled=false;
    bool NMEAout=false;
    bool NMEAin=false;
    GwNmeaFilter* readFilter=NULL;
    GwNmeaFilter* writeFilter=NULL;
    bool seaSmartOut=false;
    bool toN2k=false;
    GwLog *logger;
    String name;
    GwCounter<String> *countIn=NULL;
    GwCounter<String> *countOut=NULL;
    void updateCounter(const char *msg, bool out);
    public:
    GwChannelConfig(
        GwLog *logger,
        String name);
    void begin(
        bool enabled,
        bool nmeaOut,
        bool nmeaIn,
        String readFilter,
        String writeFilter,
        bool seaSmartOut,
        bool toN2k
    );

    void enable(bool enabled){
        this->enabled=enabled;
    }
    bool isEnabled(){return enabled;}
    bool shouldRead(){return enabled && NMEAin;}
    bool canSendOut(unsigned long pgn);
    bool canReceive(unsigned long pgn);
    bool canSendOut(const char *buffer);
    bool canReceive(const char *buffer);
    bool sendSeaSmart(){ return seaSmartOut;}
    bool sendToN2K(){return toN2k;}
    int getJsonSize();
    void toJson(GwJsonDocument &doc);
    String toString();
};

