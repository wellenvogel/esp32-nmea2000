#include "GwSocketServer.h"
#include <ESPmDNS.h>
#include <lwip/sockets.h>
#include "GwBuffer.h"

class Writer : public GwBufferWriter{
    public:
    wiFiClientPtr client;
    bool writeError=false;
    bool timeOut=false;
    unsigned long writeTimeout;
    unsigned long lastWrite;
    bool pending;
    Writer(wiFiClientPtr client, unsigned long writeTimeout=10000){
        this->client=client;
        pending=false;
        this->writeTimeout=writeTimeout;
    }
    virtual ~Writer(){}
    virtual int write(const uint8_t *buffer,size_t len){
        int res = send(client->fd(), (void*) buffer, len, MSG_DONTWAIT);
        if (res < 0){
            if (errno != EAGAIN){
                writeError=true;
                return res;
            }
            res=0;
        }
        if (res >= len){
            pending=false;
        }
        else{
            if (!pending){
                lastWrite=millis();
                pending=true;
            }
            else{
                //we need to check if we have still not been able 
                //to write until timeout
                if (millis() >= (lastWrite+writeTimeout)){
                    timeOut=true;
                }
            }
        }
        return res;
    }
};
class GwClient{
    public:
        wiFiClientPtr client;
        GwBuffer *buffer;
        GwLog *logger;
        int overflows;
        String remoteIp;
    private:
        Writer *writer=NULL;
    public:        
        GwClient(wiFiClientPtr client,GwLog *logger){
            this->client=client;
            this->logger=logger;
            buffer=new GwBuffer(logger);
            overflows=0;
            if (client != NULL){
                writer=new Writer(client);
                remoteIp=client->remoteIP().toString();
            }
        }
        void setClient(wiFiClientPtr client){
            this->client=client;
            buffer->reset();
            overflows=0;
            if (writer) delete writer;
            writer=NULL;
            if (client){
                writer=new Writer(client);
                remoteIp=client->remoteIP().toString();
            }
            else{
                remoteIp=String("---");
            }
        }
        bool hasClient(){
            return client != NULL;
        }
        ~GwClient(){
            delete writer;
        }
        bool enqueue(uint8_t *data, size_t len){
            if (len == 0) return true;
            size_t rt=buffer->addData(data,len);
            if (rt < len){
                LOG_DEBUG(GwLog::LOG,"overflow on %s",remoteIp.c_str());
                overflows++;
                return false;
            }
            return true;
        }
        bool hasData(){
            return buffer->usedSpace() > 0;
        }
        GwBuffer::WriteStatus write(){
            if (! writer) {
                LOG_DEBUG(GwLog::LOG,"write called on empty client");
                return GwBuffer::ERROR;
            }
            GwBuffer::WriteStatus rt=buffer->fetchData(writer,false);
            if (rt != GwBuffer::OK){
                LOG_DEBUG(GwLog::DEBUG+1,"write returns %d on %s",rt,remoteIp.c_str());
            }
            if (writer->timeOut ){
                LOG_DEBUG(GwLog::LOG,"timeout on %s",remoteIp.c_str());
                return GwBuffer::ERROR;
            }    
            return rt;
        }
};


GwSocketServer::GwSocketServer(const GwConfigHandler *config,GwLog *logger){
    this->config=config;
    this->logger=logger;
    maxClients=config->getInt(config->maxClients);
    clients=new gwClientPtr[maxClients];
    for (int i=0;i<maxClients;i++){
        clients[i]=gwClientPtr(new GwClient(wiFiClientPtr(NULL),logger));
    }
}
void GwSocketServer::begin(){
    server=new WiFiServer(config->getInt(config->serverPort),maxClients);
    server->begin();
    logger->logString("Socket server created, port=%d",
        config->getInt(config->serverPort));
    MDNS.addService("_nmea-0183","_tcp",config->getInt(config->serverPort));    

}
void GwSocketServer::loop()
{
    WiFiClient client = server->available(); // listen for incoming clients

    if (client)
    {
        logger->logString("new client connected from %s",
                          client.remoteIP().toString().c_str());
        fcntl(client.fd(), F_SETFL, O_NONBLOCK);
        bool canHandle = false;
        for (int i = 0; i < maxClients; i++)
        {
            if (!clients[i]->hasClient())
            {
                clients[i]->setClient(wiFiClientPtr(new WiFiClient(client)));
                logger->logString("set client as number %d", i);
                canHandle = true;
                break;
            }
        }
        if (!canHandle)
        {
            logger->logDebug(GwLog::ERROR, "no space to store client, disconnect");
            client.stop();
        }
    }
    //sending
    for (int i = 0; i < maxClients; i++)
    {
        gwClientPtr client = clients[i];
        if (!client->hasClient())
            continue;
        GwBuffer::WriteStatus rt = client->write();
        if (rt == GwBuffer::ERROR)
        {
            LOG_DEBUG(GwLog::ERROR, "write error on %s, closing", client->remoteIp.c_str());
            client->client->stop();
        }
    }
    for (int i = 0; i < maxClients; i++)
    {
        gwClientPtr client = clients[i];
        if (!client->hasClient())
            continue;

        if (!client->client->connected())
        {
            logger->logString("client %d disconnect %s", i, client->remoteIp.c_str());
            client->client->stop();
            client->setClient(NULL);
        }
        else
        {
            while (client->client->available())
            {
                char c = client->client->read();
                //TODO: read data
            }
        }
    }
}
void GwSocketServer::sendToClients(const char *buf){
    int len=strlen(buf);
    char buffer[len+2];
    memcpy(buffer,buf,len);
    buffer[len]=0x0d;
    len++;
    buffer[len]=0x0a;
    len++;
    for (int i = 0; i < maxClients; i++)
    {
        gwClientPtr client = clients[i];
        if (! client->hasClient()) continue;
        if ( client->client->connected() ) {
        bool rt=client->enqueue((uint8_t*)buffer,len);
        if (!rt){
            LOG_DEBUG(GwLog::DEBUG,"overflow in send to %s",client->remoteIp.c_str());    
        }
        
    }
  }
}

int GwSocketServer::numClients(){
    int num=0;
    for (int i = 0; i < maxClients; i++){
        if (clients[i]->hasClient()) num++;
    }
    return num;
}
GwSocketServer::~GwSocketServer(){

}