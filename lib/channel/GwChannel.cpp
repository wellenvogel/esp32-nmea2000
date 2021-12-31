#include "GwChannel.h"
#include <ActisenseReader.h>


class GwChannelMessageReceiver : public GwMessageFetcher{
  static const int bufferSize=GwBuffer::RX_BUFFER_SIZE+4;
  uint8_t buffer[bufferSize];
  uint8_t *writePointer=buffer;
  GwLog *logger;
  GwChannel *channel;
  GwChannel::NMEA0183Handler handler;
  public:
    GwChannelMessageReceiver(GwLog *logger,GwChannel *channel){
        this->logger=logger;
        this->channel=channel;
    }
    void setHandler(GwChannel::NMEA0183Handler handler){
        this->handler=handler;
    }
    virtual bool handleBuffer(GwBuffer *gwbuffer){
      size_t len=fetchMessageToBuffer(gwbuffer,buffer,bufferSize-4,'\n');
      writePointer=buffer+len;
      if (writePointer == buffer) return false;
      uint8_t *p;
      for (p=writePointer-1;p>=buffer && *p <= 0x20;p--){
        *p=0;
      }
      if (p > buffer){
        p++;
        *p=0x0d;
        p++;
        *p=0x0a;
        p++;
        *p=0;
      }
      for (p=buffer; *p != 0 && p < writePointer && *p <= 0x20;p++){}
      //very simple NMEA check
      if (*p != '!' && *p != '$'){
        LOG_DEBUG(GwLog::DEBUG,"unknown line [%d] - ignore: %s",id,(const char *)p);  
      }
      else{
        LOG_DEBUG(GwLog::DEBUG,"NMEA[%d]: %s",id,(const char *)p);
        if (channel->canReceive((const char *)p)){
            handler((const char *)p,id);
        }
      }
      writePointer=buffer;
      return true;
    }
};


GwChannel::GwChannel(GwLog *logger,
    String name,
    int sourceId){
    this->logger = logger;
    this->name=name;
    this->sourceId=sourceId;
    this->countIn=new GwCounter<String>(String("count")+name+String("in"));
    this->countOut=new GwCounter<String>(String("count")+name+String("out"));
    this->impl=NULL;
    this->receiver=new GwChannelMessageReceiver(logger,this);
    this->actisenseReader=NULL;
}
void GwChannel::begin(
    bool enabled,
    bool nmeaOut,
    bool nmeaIn,
    String readFilter,
    String writeFilter,
    bool seaSmartOut,
    bool toN2k,
    bool readActisense,
    bool writeActisense)
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
    this->readActisense=readActisense;
    this->writeActisense=writeActisense;
    if (readActisense|| writeActisense){
        channelStream=impl->getStream(false);
        if (! channelStream) {
            this->readActisense=false;
            this->writeActisense=false;
            LOG_DEBUG(GwLog::ERROR,"unable to read actisnse on %s",name.c_str());
        }
        else{
            if (readActisense){
                this->actisenseReader= new tActisenseReader();
                actisenseReader->SetReadStream(channelStream);
            }           
        }
    }
}
void GwChannel::setImpl(GwChannelInterface *impl){
    this->impl=impl;
}
void GwChannel::updateCounter(const char *msg, bool out)
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
bool GwChannel::canSendOut(unsigned long pgn){
    if (! enabled) return false;
    if (! NMEAout) return false;
    countOut->add(String(pgn)); 
    return true; 
}
bool GwChannel::canReceive(unsigned long pgn){
    if (!enabled) return false;
    if (!NMEAin) return false;
    countIn->add(String(pgn));
    return true;
}

bool GwChannel::canSendOut(const char *buffer){
    if (! enabled) return false;
    if (! NMEAout) return false;
    if (writeFilter && ! writeFilter->canPass(buffer)) return false;
    updateCounter(buffer,true);
    return true;
}

bool GwChannel::canReceive(const char *buffer){
    if (! enabled) return false;
    if (! NMEAin) return false;
    if (readFilter && ! readFilter->canPass(buffer)) return false;
    updateCounter(buffer,false);
    return true;
}

int GwChannel::getJsonSize(){
    if (! enabled) return 0;
    int rt=2;
    if (countIn) rt+=countIn->getJsonSize();
    if (countOut) rt+=countOut->getJsonSize();
    return rt;
}
void GwChannel::toJson(GwJsonDocument &doc){
    if (! enabled) return;
    if (countOut) countOut->toJson(doc);
    if (countIn) countIn->toJson(doc);
}
String GwChannel::toString(){
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
void GwChannel::loop(bool handleRead, bool handleWrite){
    if (! enabled || ! impl) return;
    impl->loop(handleRead,handleWrite);
}
void GwChannel::readMessages(GwChannel::NMEA0183Handler handler){
    if (! enabled || ! impl) return;
    if (readActisense || ! NMEAin) return;
    receiver->id=sourceId; 
    receiver->setHandler(handler);
    impl->readMessages(receiver);
}
void GwChannel::sendToClients(const char *buffer, int sourceId){
    if (! impl) return;
    if (canSendOut(buffer)){
        impl->sendToClients(buffer,sourceId);
    }
}
void GwChannel::parseActisense(N2kHandler handler){
    if (!enabled || ! impl || ! readActisense || ! actisenseReader) return;
    tN2kMsg N2kMsg;

    while (actisenseReader->GetMessageFromStream(N2kMsg)) {
      canReceive(N2kMsg.PGN);
      handler(N2kMsg,sourceId);
    }
}

void GwChannel::sendActisense(const tN2kMsg &msg){
    if (!enabled || ! impl || ! writeActisense || ! channelStream) return;
    canSendOut(msg.PGN);
    msg.SendInActisenseFormat(channelStream);
}
