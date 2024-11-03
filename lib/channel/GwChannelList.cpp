#include "GwChannelList.h"
#include "GwApi.h"
//check for duplicate groove usages
#define GW_PINDEFS
#include "GwHardware.h"
#include "GwSocketServer.h"
#include "GwSerial.h"
#include "GwTcpClient.h"
class SerInit{
    public:
        int serial=-1;
        int rx=-1;
        int tx=-1;
        int mode=-1;
        int fixedBaud=-1;
        SerInit(int s,int r,int t, int m, int b=-1):
            serial(s),rx(r),tx(t),mode(m),fixedBaud(b){}
};
std::vector<SerInit> serialInits;

static int typeFromMode(const char *mode){
    if (strcmp(mode,"UNI") == 0) return GWSERIAL_TYPE_UNI;
    if (strcmp(mode,"BI") == 0) return GWSERIAL_TYPE_BI;
    if (strcmp(mode,"RX") == 0) return GWSERIAL_TYPE_RX;
    if (strcmp(mode,"TX") == 0) return GWSERIAL_TYPE_TX;
    return GWSERIAL_TYPE_UNK;
}

#define CFG_SERIAL(ser,...) \
    __MSG("serial config " #ser); \
    static GwInitializer<SerInit> __serial ## ser ## _init \
        (serialInits,SerInit(ser,__VA_ARGS__));
#ifdef _GWI_SERIAL1
    CFG_SERIAL(0,_GWI_SERIAL1)
#endif
#ifdef _GWI_SERIAL2
    CFG_SERIAL(1,_GWI_SERIAL2)
#endif
//handle separate defines
    //serial 1
    #ifndef GWSERIAL_TX
      #define GWSERIAL_TX -1
    #endif
    #ifndef GWSERIAL_RX
      #define GWSERIAL_RX -1
    #endif
    #ifdef GWSERIAL_TYPE
        CFG_SERIAL(0,GWSERIAL_RX,GWSERIAL_TX,GWSERIAL_TYPE)
    #else
        #ifdef GWSERIAL_MODE
            CFG_SERIAL(0,GWSERIAL_RX,GWSERIAL_TX,typeFromMode(GWSERIAL_MODE))
        #endif
    #endif
    //serial 2
    #ifndef GWSERIAL2_TX
      #define GWSERIAL2_TX -1
    #endif
    #ifndef GWSERIAL2_RX
      #define GWSERIAL2_RX -1
    #endif
    #ifdef GWSERIAL2_TYPE
        CFG_SERIAL(1,GWSERIAL2_RX,GWSERIAL2_TX,GWSERIAL2_TYPE)
    #else
        #ifdef GWSERIAL2_MODE
            CFG_SERIAL(1,GWSERIAL2_RX,GWSERIAL2_TX,typeFromMode(GWSERIAL2_MODE))
        #endif
    #endif
class GwSerialLog : public GwLogWriter
{
    static const size_t bufferSize = 4096;
    char *logBuffer = NULL;
    int wp = 0;
    GwSerial *writer;
    bool disabled = false;
public:
    GwSerialLog(GwSerial *writer, bool disabled)
    {
        this->writer = writer;
        this->disabled = disabled;
        logBuffer = new char[bufferSize];
        wp = 0;
    }
    virtual ~GwSerialLog() {}
    virtual void write(const char *data)
    {
        if (disabled)
            return;
        int len = strlen(data);
        if ((wp + len) >= (bufferSize - 1))
            return;
        strncpy(logBuffer + wp, data, len);
        wp += len;
        logBuffer[wp] = 0;
    }
    virtual void flush()
    {
        size_t handled = 0;
        if (!disabled)
        {
            while (handled < wp)
            {
                if ( !writer->flush()) break;
                size_t rt = writer->sendToClients(logBuffer + handled, -1, true);
                handled += rt;
            }
            if (handled < wp){
                if (handled > 0){
                    memmove(logBuffer,logBuffer+handled,wp-handled);
                    wp-=handled;
                    logBuffer[wp]=0;
                }
                return;
            }
        }
        wp = 0;
        logBuffer[0] = 0;
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
typedef struct {
    int id;
    const char *baud;
    const char *receive;
    const char *send;
    const char *direction;
    const char *toN2K;
    const char *readF;
    const char *writeF;
    const char *preventLog;
    const char *readAct;
    const char *writeAct;
    const char *name;
} SerialParam;

static  SerialParam serialParameters[]={
    {
        .id=USB_CHANNEL_ID,
        .baud=GwConfigDefinitions::usbBaud,
        .receive=GwConfigDefinitions::receiveUsb,
        .send=GwConfigDefinitions::sendUsb,
        .direction="",
        .toN2K=GwConfigDefinitions::usbToN2k,
        .readF=GwConfigDefinitions::usbReadFilter,
        .writeF=GwConfigDefinitions::usbWriteFilter,
        .preventLog=GwConfigDefinitions::usbActisense,
        .readAct=GwConfigDefinitions::usbActisense,
        .writeAct=GwConfigDefinitions::usbActSend,
        .name="USB"
    },
    {
        .id=SERIAL1_CHANNEL_ID,
        .baud=GwConfigDefinitions::serialBaud,
        .receive=GwConfigDefinitions::receiveSerial,
        .send=GwConfigDefinitions::sendSerial,
        .direction=GwConfigDefinitions::serialDirection,
        .toN2K=GwConfigDefinitions::serialToN2k,
        .readF=GwConfigDefinitions::serialReadF,
        .writeF=GwConfigDefinitions::serialWriteF,
        .preventLog="",
        .readAct="",
        .writeAct="",
        .name="Serial"
    },
    {
        .id=SERIAL2_CHANNEL_ID,
        .baud=GwConfigDefinitions::serial2Baud,
        .receive=GwConfigDefinitions::receiveSerial2,
        .send=GwConfigDefinitions::sendSerial2,
        .direction=GwConfigDefinitions::serial2Dir,
        .toN2K=GwConfigDefinitions::serial2ToN2k,
        .readF=GwConfigDefinitions::serial2ReadF,
        .writeF=GwConfigDefinitions::serial2WriteF,
        .preventLog="",
        .readAct="",
        .writeAct="",
        .name="Serial2"
    }
};

template<typename T>
GwSerial* createSerial(GwLog *logger, T* s,int id, bool canRead=true){
    return new GwSerialImpl<T>(logger,s,id,canRead);
} 

static GwChannel * createSerialChannel(GwConfigHandler *config,GwLog *logger, int idx,int type,int rx,int tx, bool setLog=false){
    if (idx < 0 || idx >= sizeof(serialParameters)/sizeof(SerialParam*)) return nullptr;
    SerialParam *param=&serialParameters[idx];
    bool canRead=false;
    bool canWrite=false;
    bool validType=false;
    if (type == GWSERIAL_TYPE_BI){
        canRead=config->getBool(param->receive);
        canWrite=config->getBool(param->send);
        validType=true;
    }
    if (type == GWSERIAL_TYPE_TX){
        canWrite=true;
        validType=true;
    }
    if (type == GWSERIAL_TYPE_RX){
        canRead=true;
        validType=true;
    }
    if (type == GWSERIAL_TYPE_UNI ){
        String cfgMode=config->getString(param->direction);
        if (cfgMode == "receive"){
            canRead=true;
        }
        if (cfgMode == "send"){
            canWrite=true;
        }
        validType=true;
    }
    if (! validType){
        LOG_DEBUG(GwLog::ERROR,"invalid type for serial channel %d: %d",param->id,type);
        return nullptr;
    }
    if (rx < 0) canRead=false;
    if (tx < 0) canWrite=false;
    LOG_DEBUG(GwLog::DEBUG,"serial set up: type=%d,rx=%d,canRead=%d,tx=%d,canWrite=%d",
        type,rx,(int)canRead,tx,(int)canWrite);
    GwSerial *serialStream=nullptr;
    GwLog *streamLog=setLog?nullptr:logger;
    switch(param->id){
        case USB_CHANNEL_ID:
            serialStream=createSerial(streamLog,&USBSerial,param->id);
            break;
        case SERIAL1_CHANNEL_ID:
            serialStream=createSerial(streamLog,&Serial1,param->id);
            break;
        case SERIAL2_CHANNEL_ID:
            serialStream=createSerial(streamLog,&Serial2,param->id);
            break;
    }
    if (serialStream == nullptr){
        LOG_DEBUG(GwLog::ERROR,"invalid serial config with id %d",param->id);
        return nullptr;
    }
    serialStream->begin(config->getInt(param->baud,115200),SERIAL_8N1,rx,tx);
    if (setLog){
        logger->setWriter(new GwSerialLog(serialStream,config->getBool(param->preventLog,false)));
        logger->prefix="GWSERIAL:";
    }
    LOG_DEBUG(GwLog::LOG, "starting serial %d ", param->id);
    GwChannel *channel = new GwChannel(logger, param->name,param->id);
    channel->setImpl(serialStream);
    channel->begin(
        canRead || canWrite,
        canWrite,
        canRead,
        config->getString(param->readF),
        config->getString(param->writeF),
        false,
        config->getBool(param->toN2K),
        config->getBool(param->readAct),
        config->getBool(param->writeAct));
    return channel;
}
void GwChannelList::addChannel(GwChannel * channel){
    for (auto &&it:theChannels){
        if (it->overlaps(channel)){
            LOG_DEBUG(GwLog::ERROR,"trying to add channel with overlapping ids %s (%s), ignoring",
                channel->toString().c_str(),
                it->toString().c_str());
            return;
        }
    }
    LOG_DEBUG(GwLog::LOG, "adding channel %s", channel->toString().c_str());
    theChannels.push_back(channel);
}
void GwChannelList::preinit(){
    for (auto &&init:serialInits){
        if (init.fixedBaud >= 0){
            switch(init.serial){
              case 1:
              {
                LOG_DEBUG(GwLog::DEBUG,"setting fixed baud %d for serial",init.fixedBaud);
                config->setValue(GwConfigDefinitions::serialBaud,String(init.fixedBaud),GwConfigInterface::READONLY);
              }
              break;
              case 2:
              {
                LOG_DEBUG(GwLog::DEBUG,"setting fixed baud %d for serial2",init.fixedBaud);
                config->setValue(GwConfigDefinitions::serial2Baud,String(init.fixedBaud),GwConfigInterface::READONLY);
              }
              break;
              default:
                LOG_DEBUG(GwLog::ERROR,"invalid serial definition %d found",init.serial)
            }
        }
    }
}
#ifndef GWUSB_TX
  #define GWUSB_TX -1
#endif
#ifndef GWUSB_RX
  #define GWUSB_RX -1
#endif

void GwChannelList::begin(bool fallbackSerial){
    LOG_DEBUG(GwLog::DEBUG,"GwChannelList::begin");
    GwChannel *channel=NULL;
    //usb
    if (! fallbackSerial){
        GwChannel *usb=createSerialChannel(config, logger,0,GWSERIAL_TYPE_BI,GWUSB_RX,GWUSB_TX,true);
        addChannel(usb);
    }
    //TCP server
    sockets=new GwSocketServer(config,logger,MIN_TCP_CHANNEL_ID);
    sockets->begin();
    channel=new GwChannel(logger,"TCPserver",MIN_TCP_CHANNEL_ID,MIN_TCP_CHANNEL_ID+10);
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
    addChannel(channel);

    //new serial config handling
    for (auto &&init:serialInits){
        (logger,init.serial,init.rx,init.tx,init.mode);
    }
    
    //tcp client
    bool tclEnabled=config->getBool(config->tclEnabled);
    channel=new GwChannel(logger,"TCPClient",TCP_CLIENT_CHANNEL_ID);
    if (tclEnabled){
        client=new GwTcpClient(logger);
        client->begin(TCP_CLIENT_CHANNEL_ID,
            config->getString(config->remoteAddress),
            config->getInt(config->remotePort),
            config->getBool(config->readTCL)
        );
        channel->setImpl(client);
    }
    channel->begin(
        tclEnabled,
        config->getBool(config->sendTCL),
        config->getBool(config->readTCL),
        config->getString(config->tclReadFilter),
        config->getString(config->tclReadFilter),
        config->getBool(config->tclSeasmart),
        config->getBool(config->tclToN2k),
        false,
        false
        );
    addChannel(channel);
    logger->flush();
}
String GwChannelList::getMode(int id){
    for (auto && c: theChannels){
        if (c->isOwnSource(id)) return c->getMode();
    }
    return "UNKNOWN";
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
    else{
        doc["clientCon"]=false;
        doc["clientErr"]="disabled";
    }
    allChannels([&](GwChannel *c){
        c->toJson(doc);
    });
}
GwChannel *GwChannelList::getChannelById(int sourceId){
    for (auto && it: theChannels){
        if (it->isOwnSource(sourceId)) return it;
    }
    return NULL;
}

void GwChannelList::fillStatus(GwApi::Status &status){
    GwChannel *channel=getChannelById(USB_CHANNEL_ID);
    if (channel){
        status.usbRx=channel->countRx();
        status.usbTx=channel->countTx();
    }
    channel=getChannelById(SERIAL1_CHANNEL_ID);
    if (channel){
        status.serRx=channel->countRx();
        status.serTx=channel->countTx();
    }
    channel=getChannelById(SERIAL2_CHANNEL_ID);
    if (channel){
        status.ser2Rx=channel->countRx();
        status.ser2Tx=channel->countTx();
    }
    channel=getChannelById(MIN_TCP_CHANNEL_ID);
    if (channel){
        status.tcpSerRx=channel->countRx();
        status.tcpSerTx=channel->countTx();
    }
    channel=getChannelById(TCP_CLIENT_CHANNEL_ID);
    if (channel){
        status.tcpClRx=channel->countRx();
        status.tcpClTx=channel->countTx();
    }
}