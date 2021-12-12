#include "GwSocketServer.h"
#include <ESPmDNS.h>
#include <lwip/sockets.h>
#include "GwBuffer.h"

class GwClient{
    public:
        wiFiClientPtr client;
        int overflows;
        String remoteIp;
    private:
        unsigned long lastWrite=0;
        unsigned long writeTimeout=10000;
        bool pendingWrite=false;
        bool writeError=false; 
        bool allowRead;
        GwBuffer *buffer=NULL;
        GwBuffer *readBuffer=NULL;
        GwLog *logger;
    public:        
        GwClient(wiFiClientPtr client,GwLog *logger,int id, bool allowRead=false){
            this->client=client;
            this->logger=logger;
            this->allowRead=allowRead;
            String bufName="Sock(";
            bufName+=String(id);
            bufName+=")";
            buffer=new GwBuffer(logger,GwBuffer::TX_BUFFER_SIZE,bufName+"wr");
            if (allowRead){
                readBuffer=new GwBuffer(logger,GwBuffer::RX_BUFFER_SIZE,bufName+"rd");
            }
            overflows=0;
            if (client != NULL){
                remoteIp=client->remoteIP().toString();
            }
        }
        void setClient(wiFiClientPtr client){
            this->client=client;
            buffer->reset("new client");
            if (readBuffer) readBuffer->reset("new client");
            overflows=0;
            pendingWrite=false;
            writeError=false;
            lastWrite=0;
            if (client){
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
            delete buffer;
            if (readBuffer) delete readBuffer;
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
        bool handleError(int res,bool errorIf0=true){
            if (res == 0 && errorIf0){
                LOG_DEBUG(GwLog::LOG,"client shutdown (recv 0) on %s",remoteIp.c_str());
                client->stop();
                return false;
            }
            if (res < 0){
                if (errno != EAGAIN){
                    LOG_DEBUG(GwLog::LOG,"client read error %d on %s",errno,remoteIp.c_str());
                    client->stop();
                    return false;
                }
                return false;
            }
            return true;
        }
        GwBuffer::WriteStatus write(){
            if (! hasClient()) {
                LOG_DEBUG(GwLog::LOG,"write called on empty client");
                return GwBuffer::ERROR;
            }
            if (! buffer->usedSpace()){
                pendingWrite=false;
                return GwBuffer::OK;
            }
            buffer->fetchData(-1,[](uint8_t *buffer, size_t len, void *param)->size_t{
                GwClient *c=(GwClient*)param;
                int res = send(c->client->fd(), (void*) buffer, len, MSG_DONTWAIT);
                if (! c->handleError(res,false)) return 0;
                if (res >= len){
                    c->pendingWrite=false;
                }
                else{
                    if (!c->pendingWrite){
                        c->lastWrite=millis();
                        c->pendingWrite=true;
                    }
                    else{
                        //we need to check if we have still not been able 
                        //to write until timeout
                        if (millis() >= (c->lastWrite+c->writeTimeout)){
                            c->logger->logDebug(GwLog::ERROR,"Write timeout on channel %s",c->remoteIp.c_str());
                            c->writeError=true;
                        }
                    }
                }
                return res;
            },this);
            if (writeError){
                LOG_DEBUG(GwLog::DEBUG+1,"write error on %s",remoteIp.c_str());
                return GwBuffer::ERROR;
            }
                
            return GwBuffer::OK;
        }
        
        bool read(){
            if (! allowRead){
                size_t maxLen=100;
                char buffer[maxLen];
                int res = recv(client->fd(), (void*) buffer, maxLen, MSG_DONTWAIT);
                return handleError(res);
            }
            readBuffer->fillData(-1,[](uint8_t *buffer, size_t len, void *param)->size_t{
                GwClient *c=(GwClient*)param;
                int res = recv(c->client->fd(), (void*) buffer, len, MSG_DONTWAIT);
                if (! c->handleError(res)) return 0;
                return res;
            },this);
            return true;
        }
        bool messagesFromBuffer(GwMessageFetcher *writer){
            if (! allowRead) return false;
            return writer->handleBuffer(readBuffer);
        }
};


GwSocketServer::GwSocketServer(const GwConfigHandler *config,GwLog *logger,int minId){
    this->config=config;
    this->logger=logger;
    this->minId=minId;
    maxClients=1;
    allowReceive=false;
}
void GwSocketServer::begin(){
    maxClients=config->getInt(config->maxClients);
    allowReceive=config->getBool(config->readTCP);
    clients=new gwClientPtr[maxClients];
    for (int i=0;i<maxClients;i++){
        clients[i]=gwClientPtr(new GwClient(wiFiClientPtr(NULL),logger,i,allowReceive));
    }
    server=new WiFiServer(config->getInt(config->serverPort),maxClients+1);
    server->begin();
    logger->logString("Socket server created, port=%d",
        config->getInt(config->serverPort));
    MDNS.addService("_nmea-0183","_tcp",config->getInt(config->serverPort));    

}
void GwSocketServer::loop(bool handleRead,bool handleWrite)
{
    if (! clients) return;
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
    if (handleWrite)
    {
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
            if (handleRead) client->read();
        }
    }
}

bool GwSocketServer::readMessages(GwMessageFetcher *writer){
    if (! allowReceive || ! clients) return false;
    bool hasMessages=false;
    for (int i = 0; i < maxClients; i++){
        writer->id=minId+i;
        if (!clients[i]->hasClient()) continue;
        if (clients[i]->messagesFromBuffer(writer)) hasMessages=true;
    }
    return hasMessages;
}
void GwSocketServer::sendToClients(const char *buf,int source){
    if (! clients) return;
    int len=strlen(buf);
    int sourceIndex=source-minId;
    for (int i = 0; i < maxClients; i++)
    {
        if (i == sourceIndex)continue; //never send out to the source we received from
        gwClientPtr client = clients[i];
        if (! client->hasClient()) continue;
        if ( client->client->connected() ) {
            client->enqueue((uint8_t*)buf,len);
        }
    }
}

int GwSocketServer::numClients(){
    if (! clients) return 0;
    int num=0;
    for (int i = 0; i < maxClients; i++){
        if (clients[i]->hasClient()) num++;
    }
    return num;
}
GwSocketServer::~GwSocketServer(){

}