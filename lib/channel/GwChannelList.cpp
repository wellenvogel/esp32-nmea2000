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

#define CFG_SERIAL(ser,...) \
    __MSG("serial config " #ser); \
    static GwInitializer<SerInit> __serial ## ser ## _init \
        (serialInits,SerInit(ser,__VA_ARGS__));
#ifdef _GWI_SERIAL1
    CFG_SERIAL(1,_GWI_SERIAL1)
#endif
#ifdef _GWI_SERIAL2
    CFG_SERIAL(2,_GWI_SERIAL2)
#endif
class GwSerialLog : public GwLogWriter
{
    static const size_t bufferSize = 4096;
    char *logBuffer = NULL;
    int wp = 0;
    GwSerial *writer;
    bool disabled = false;
    long flushTimeout=200;
public:
    GwSerialLog(GwSerial *writer, bool disabled,long flushTimeout=200)
    {
        this->writer = writer;
        this->disabled = disabled;
        this->flushTimeout=flushTimeout;
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
                if ( !writer->flush(flushTimeout)) break;
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

template<typename T>
    class SerialWrapper : public GwChannelList::SerialWrapperBase{
        private:
        template<class C>
        void beginImpl(C *s,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1){}
        void beginImpl(HardwareSerial *s,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1){
            s->begin(baud,config,rxPin,txPin);
        }
        template<class C>
        void setError(C* s, GwLog *logger){}
        void setError(HardwareSerial *s,GwLog *logger){
            LOG_DEBUG(GwLog::LOG,"enable serial errors for channel %d",id);
            s->onReceiveError([logger,this](hardwareSerial_error_t err){
                LOG_DEBUG(GwLog::ERROR,"serial error on id %d: %d",this->id,(int)err);
            });
        }
        #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
            void beginImpl(HWCDC *s,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1){
            s->begin(baud);
        }
        #endif
        T *serial;
        int id;
        public:
        SerialWrapper(T* s,int i):serial(s),id(i){}
        virtual void begin(GwLog* logger,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1) override{
            beginImpl(serial,baud,config,rxPin,txPin);
            setError(serial,logger);
        };
        virtual Stream *getStream() override{
            return serial;
        }
        virtual int getId() override{
            return id;
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
    const char *name;
} SerialParam;

static  SerialParam serialParameters[]={
    {
        .id=SERIAL1_CHANNEL_ID,
        .baud=GwConfigDefinitions::serialBaud,
        .receive=GwConfigDefinitions::receiveSerial,
        .send=GwConfigDefinitions::sendSerial,
        .direction=GwConfigDefinitions::serialDirection,
        .toN2K=GwConfigDefinitions::serialToN2k,
        .readF=GwConfigDefinitions::serialReadF,
        .writeF=GwConfigDefinitions::serialWriteF,
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
        .name="Serial2"
    }
};

static SerialParam *getSerialParam(int id){
    for (size_t idx=0;idx<sizeof(serialParameters)/sizeof(SerialParam*);idx++){
        if (serialParameters[idx].id == id) return &serialParameters[idx];
    }
    return nullptr;
}
void GwChannelList::addSerial(int id, int rx, int tx, int type){
    if (id == 1){
        addSerial(new SerialWrapper<decltype(Serial1)>(&Serial1,SERIAL1_CHANNEL_ID),type,rx,tx);
        return;   
    }
    if (id == 2){
        addSerial(new SerialWrapper<decltype(Serial2)>(&Serial2,SERIAL2_CHANNEL_ID),type,rx,tx);
        return;   
    }
    LOG_DEBUG(GwLog::ERROR,"invalid serial config with id %d",id);
}
void GwChannelList::addSerial(GwChannelList::SerialWrapperBase *stream,int type,int rx,int tx){
    const char *mode=nullptr;
    switch (type)
    {
    case GWSERIAL_TYPE_UNI:
        mode="UNI";
        break;
    case GWSERIAL_TYPE_BI:
        mode="BI";
        break;
    case GWSERIAL_TYPE_RX:
        mode="RX";
        break;
    case GWSERIAL_TYPE_TX:
        mode="TX";
        break;
    }
    if (mode == nullptr) {
        LOG_DEBUG(GwLog::ERROR,"unknown serial type %d",type);
        return;
    }
    addSerial(stream,mode,rx,tx);
}
void GwChannelList::addSerial(GwChannelList::SerialWrapperBase *serialStream,const String &mode,int rx,int tx){
    int id=serialStream->getId();
    for (auto &&it:theChannels){
        if (it->isOwnSource(id)){
            LOG_DEBUG(GwLog::ERROR,"trying to re-add serial id=%d, ignoring",id);
            return;
        }
    }
    SerialParam *param=getSerialParam(id);
    if (param == nullptr){
        logger->logDebug(GwLog::ERROR,"trying to set up an unknown serial channel: %d",id);
        return;
    }
    if (rx < 0 && tx < 0){
        logger->logDebug(GwLog::ERROR,"useless config for serial %d: both rx/tx undefined");
        return;
    }
    modes[id]=String(mode);
    bool canRead=false;
    bool canWrite=false;
    if (mode == "BI"){
        canRead=config->getBool(param->receive);
        canWrite=config->getBool(param->send);
    }
    if (mode == "TX"){
        canWrite=true;
    }
    if (mode == "RX"){
        canRead=true;
    }
    if (mode == "UNI"){
        String cfgMode=config->getString(param->direction);
        if (cfgMode == "receive"){
            canRead=true;
        }
        if (cfgMode == "send"){
            canWrite=true;
        }
    }
    if (rx < 0) canRead=false;
    if (tx < 0) canWrite=false;
    LOG_DEBUG(GwLog::DEBUG,"serial set up: mode=%s,rx=%d,canRead=%d,tx=%d,canWrite=%d",
        mode.c_str(),rx,(int)canRead,tx,(int)canWrite);
    serialStream->begin(logger,config->getInt(param->baud,115200),SERIAL_8N1,rx,tx);
    GwSerial *serial = new GwSerial(logger, serialStream->getStream(), id, canRead);
    LOG_DEBUG(GwLog::LOG, "starting serial %d ", id);
    GwChannel *channel = new GwChannel(logger, param->name, id);
    channel->setImpl(serial);
    channel->begin(
        canRead || canWrite,
        canWrite,
        canRead,
        config->getString(param->readF),
        config->getString(param->writeF),
        false,
        config->getBool(param->toN2K),
        false,
        false);
    LOG_DEBUG(GwLog::LOG, "%s", channel->toString().c_str());
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
template<typename S>
long getFlushTimeout(S &s){
    return 200;
}
template<>
long getFlushTimeout(HardwareSerial &s){
    return 2000;
}
void GwChannelList::begin(bool fallbackSerial){
    LOG_DEBUG(GwLog::DEBUG,"GwChannelList::begin");
    GwChannel *channel=NULL;
    //usb
    if (! fallbackSerial){
        GwSerial *usb=new GwSerial(NULL,&USBSerial,USB_CHANNEL_ID);
        USBSerial.begin(config->getInt(config->usbBaud));
        logger->setWriter(new GwSerialLog(usb,config->getBool(config->usbActisense),getFlushTimeout(USBSerial)));
        logger->prefix="GWSERIAL:";
        channel=new GwChannel(logger,"USB",USB_CHANNEL_ID);
        channel->setImpl(usb);
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
        theChannels.push_back(channel);
        LOG_DEBUG(GwLog::LOG,"%s",channel->toString().c_str());
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
    LOG_DEBUG(GwLog::LOG,"%s",channel->toString().c_str());
    theChannels.push_back(channel);

    //new serial config handling
    for (auto &&init:serialInits){
        addSerial(init.serial,init.rx,init.tx,init.mode);
    }
    //handle separate defines
    //serial 1
    #ifndef GWSERIAL_TX
      #define GWSERIAL_TX -1
    #endif
    #ifndef GWSERIAL_RX
      #define GWSERIAL_RX -1
    #endif
    #ifdef GWSERIAL_TYPE
        addSerial(new SerialWrapper<decltype(Serial1)>(&Serial1,SERIAL1_CHANNEL_ID),GWSERIAL_TYPE,GWSERIAL_RX,GWSERIAL_TX);
    #else
        #ifdef GWSERIAL_MODE
            addSerial(new SerialWrapper<decltype(Serial1)>(&Serial1,SERIAL1_CHANNEL_ID),GWSERIAL_MODE,GWSERIAL_RX,GWSERIAL_TX);
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
        addSerial(new SerialWrapper<decltype(Serial2)>(&Serial2,SERIAL2_CHANNEL_ID),GWSERIAL2_TYPE,GWSERIAL2_RX,GWSERIAL2_TX);
    #else
        #ifdef GWSERIAL2_MODE
            addSerial(new SerialWrapper<decltype(Serial2)>(&Serial2,SERIAL2_CHANNEL_ID),GWSERIAL2_MODE,GWSERIAL2_RX,GWSERIAL2_TX);
        #endif
    #endif
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
    theChannels.push_back(channel);
    LOG_DEBUG(GwLog::LOG,"%s",channel->toString().c_str());  
    logger->flush();
}
String GwChannelList::getMode(int id){
    auto it=modes.find(id);
    if (it != modes.end()) return it->second;
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
    for (auto it=theChannels.begin();it != theChannels.end();it++){
        if ((*it)->isOwnSource(sourceId)) return *it;
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