#pragma once
#include "GwChannelInterface.h"
#include "GwConfigItem.h"
#include "GwLog.h"
#include "GWConfig.h"
#include "GwCounter.h"
#include "GwJsonDocument.h"
#include <N2kMsg.h>
#include <functional>

class GwChannelMessageReceiver;
class tActisenseReader;
class GwChannel{
    bool enabled=false;
    bool NMEAout=false;
    bool NMEAin=false;
    GwNmeaFilter* readFilter=NULL;
    GwNmeaFilter* writeFilter=NULL;
    bool seaSmartOut=false;
    bool toN2k=false;
    bool readActisense=false;
    bool writeActisense=false;
    GwLog *logger;
    String name;
    GwCounter<String> *countIn=NULL;
    GwCounter<String> *countOut=NULL;
    GwChannelInterface *impl;
    int sourceId=0;
    int maxSourceId=-1;
    GwChannelMessageReceiver *receiver=NULL;
    tActisenseReader *actisenseReader=NULL;
    Stream *channelStream=NULL;
    void updateCounter(const char *msg, bool out);
    public:
    GwChannel(
        GwLog *logger,
        String name,
        int sourceId,
        int maxSourceId=-1);
    void begin(
        bool enabled,
        bool nmeaOut,
        bool nmeaIn,
        String readFilter,
        String writeFilter,
        bool seaSmartOut,
        bool toN2k,
        bool readActisense=false,
        bool writeActisense=false
    );

    void setImpl(GwChannelInterface *impl);
    bool isOwnSource(int id);
    void enable(bool enabled){
        this->enabled=enabled;
    }
    bool isEnabled(){return enabled;}
    bool shouldRead(){return enabled && NMEAin;}
    bool canSendOut(const char *buffer);
    bool canReceive(const char *buffer);
    bool sendSeaSmart(){ return seaSmartOut;}
    bool sendToN2K(){return toN2k;}
    int getJsonSize();
    void toJson(GwJsonDocument &doc);
    String toString();

    void loop(bool handleRead, bool handleWrite);
    typedef std::function<void(const char *buffer, int sourceid)> NMEA0183Handler;
    void readMessages(NMEA0183Handler handler);
    void sendToClients(const char *buffer, int sourceId);
    typedef std::function<void(const tN2kMsg &msg, int sourceId)> N2kHandler ;
    void parseActisense(N2kHandler handler);
    void sendActisense(const tN2kMsg &msg, int sourceId);
    unsigned long countRx();
    unsigned long countTx();
};

