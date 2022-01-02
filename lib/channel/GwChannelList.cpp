#include "GwChannelList.h"
#include "GwApi.h"
#include "GwHardware.h"
#include "GwSocketServer.h"
#include "GwSerial.h"
#include "GwTcpClient.h"
class GwSerialLog : public GwLogWriter{
  static const size_t bufferSize=4096;
  char *logBuffer=NULL;
  int wp=0;
  GwSerial *writer;
  public:
    GwSerialLog(GwSerial *writer){
        this->writer=writer;
      logBuffer=new char[bufferSize];
      wp=0;
    }
    virtual ~GwSerialLog(){}
    virtual void write(const char *data){
      int len=strlen(data);
      if ((wp+len) >= (bufferSize-1)) return;
      strncpy(logBuffer+wp,data,len);
      wp+=len;
      logBuffer[wp]=0;
    }
    virtual void flush(){
      size_t handled=0;
      while (handled < wp){
        writer->flush();
        size_t rt=writer->sendToClients(logBuffer+handled,-1,true);
        handled+=rt;
        }
      wp=0;
      logBuffer[0]=0;
    }

};


GwChannelList::GwChannelList(GwLog *logger, GwConfigHandler *config){
    this->logger=logger;
    this->config=config;
}
void GwChannelList::allChannels(ChannelAction action){
    for (auto it=theChannels.begin();it != theChannels.end();it++){
        action(*it);
    }
}
void GwChannelList::begin(bool fallbackSerial){
    LOG_DEBUG(GwLog::DEBUG,"GwChannelList::begin");
    GwChannel *channel=NULL;
    //usb
    if (! fallbackSerial){
        GwSerial *usb=new GwSerial(NULL,0,USB_CHANNEL_ID);
        usb->setup(config->getInt(config->usbBaud),3,1);
        logger->setWriter(new GwSerialLog(usb));
        logger->prefix="GWSERIAL:";
        channel=new GwChannel(logger,"USB",USB_CHANNEL_ID);
        channel->begin(true,
            config->getBool(config->sendUsb),
            config->getBool(config->receiveUsb),
            config->getString(config->usbReadFilter),
            config->getString(config->usbWriteFilter),
            false,
            config->getBool(config->usbToN2k),
            config->getBool(config->usbActisense),
            config->getBool(config->usbActSend)
        );
        LOG_DEBUG(GwLog::LOG,"%s",channel->toString().c_str());
    }
    //TCP server
    sockets=new GwSocketServer(config,logger,MIN_TCP_CHANNEL_ID);
    sockets->begin();
    channel=new GwChannel(logger,"TCP",MIN_TCP_CHANNEL_ID,MIN_TCP_CHANNEL_ID+10);
    channel->setImpl(sockets);
    channel->begin(
        true,
        config->getBool(config->sendTCP),
        config->getBool(config->readTCP),
        config->getString(config->tcpReadFilter),
        config->getString(config->tcpWriteFilter),
        config->getBool(config->sendSeasmart),
        config->getBool(config->tcpToN2k),
        false,
        false
    );
    LOG_DEBUG(GwLog::LOG,"%s",channel->toString().c_str());
    theChannels.push_back(channel);

    //serial 1
    bool serCanRead=false;
    bool serCanWrite=false;
    int serialrx=-1;
    int serialtx=-1;
    #ifdef GWSERIAL_MODE
        #ifdef GWSERIAL_TX
            serialtx=GWSERIAL_TX;
        #endif
        #ifdef GWSERIAL_RX
            serialrx=GWSERIAL_RX;
        #endif
        if (serialrx != -1 && serialtx != -1){
            serialMode=GWSERIAL_MODE;
        }
    #endif
    //the serial direction is from the config (only valid for mode UNI)
    String serialDirection=config->getString(config->serialDirection);
    //we only consider the direction if mode is UNI
    if (serialMode != String("UNI")){
        serialDirection=String("");
        //if mode is UNI it depends on the selection
        serCanRead=config->getBool(config->receiveSerial);
        serCanWrite=config->getBool(config->sendSerial);
    }
    if (serialDirection == "receive" || serialDirection == "off" || serialMode == "RX") serCanWrite=false;
    if (serialDirection == "send" || serialDirection == "off" || serialMode == "TX") serCanRead=false;
    LOG_DEBUG(GwLog::DEBUG,"serial set up: mode=%s,direction=%s,rx=%d,tx=%d",
        serialMode.c_str(),serialDirection.c_str(),serialrx,serialtx
        );
    if (serialtx != -1 || serialrx != -1 ){
        LOG_DEBUG(GwLog::LOG,"creating serial interface rx=%d, tx=%d",serialrx,serialtx);
        GwSerial *serial=new GwSerial(logger,1,SERIAL1_CHANNEL_ID,serCanRead);
        int rt=serial->setup(config->getInt(config->serialBaud,115200),serialrx,serialtx);
        LOG_DEBUG(GwLog::LOG,"starting serial returns %d",rt);
        channel=new GwChannel(logger,"SER",SERIAL1_CHANNEL_ID);
        channel->setImpl(serial);
        channel->begin(
            serCanRead || serCanWrite,
            serCanWrite,
            serCanRead,
            config->getString(config->serialReadF),
            config->getString(config->serialWriteF),
            false,
            config->getBool(config->serialToN2k),
            false,
            false    
        );
        LOG_DEBUG(GwLog::LOG,"%s",channel->toString().c_str());
        theChannels.push_back(channel);
    }

    //tcp client
    channel=new GwChannel(logger,"TCPClient",TCP_CLIENT_CHANNEL_ID);
    channel->begin(
        config->getBool(config->tclEnabled),
        config->getBool(config->sendTCL),
        config->getBool(config->readTCL),
        config->getString(config->tclReadFilter),
        config->getString(config->tclReadFilter),
        config->getBool(config->tclSeasmart),
        config->getBool(config->tclToN2k),
        false,
        false
        );
    if (channel->isEnabled()){
        client=new GwTcpClient(logger);
        client->begin(TCP_CLIENT_CHANNEL_ID,
            config->getString(config->remoteAddress),
            config->getInt(config->remotePort),
            channel->shouldRead()
        );
        channel->setImpl(client);
    }
    LOG_DEBUG(GwLog::LOG,"%s",channel->toString().c_str());  
    logger->flush();
}
int GwChannelList::getJsonSize(){
    int rt=0;
    allChannels([&](GwChannel *c){
        rt+=c->getJsonSize();
    });
    return rt+20;
}
void GwChannelList::toJson(GwJsonDocument &doc){
    if (sockets) doc["numClients"]=sockets->numClients();
    if (client){
        doc["clientCon"]=client->isConnected();
        doc["clientErr"]=client->getError();
    }
    allChannels([&](GwChannel *c){
        c->toJson(doc);
    });
}
GwChannel *GwChannelList::getChannelById(int sourceId){
    for (auto it=theChannels.begin();it != theChannels.end();it++){
        if ((*it)->isOwnSource(sourceId)) return *it;
    }
    return NULL;
}