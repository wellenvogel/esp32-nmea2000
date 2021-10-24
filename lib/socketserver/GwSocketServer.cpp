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
    private:
        Writer *writer;
    public:        
        GwClient(wiFiClientPtr client,GwLog *logger){
            this->client=client;
            this->logger=logger;
            buffer=new GwBuffer(logger);
            overflows=0;
            writer=new Writer(client);
        }
        ~GwClient(){
            delete writer;
        }
        bool enqueue(uint8_t *data, size_t len){
            if (len == 0) return true;
            size_t rt=buffer->addData(data,len);
            if (rt < len){
                LOG_DEBUG(GwLog::LOG,"overflow on %s",client->remoteIP().toString().c_str());
                overflows++;
                return false;
            }
            return true;
        }
        bool hasData(){
            return buffer->usedSpace() > 0;
        }
        GwBuffer::WriteStatus write(){
            GwBuffer::WriteStatus rt=buffer->fetchData(writer,false);
            if (rt != GwBuffer::OK){
                LOG_DEBUG(GwLog::DEBUG+1,"write returns %d on %s",rt,client->remoteIP().toString().c_str());
            }
            if (writer->timeOut ){
                LOG_DEBUG(GwLog::LOG,"timeout on %s",client->remoteIP().toString().c_str());
                return GwBuffer::ERROR;
            }    
            return rt;
        }
};


GwSocketServer::GwSocketServer(const GwConfigHandler *config,GwLog *logger){
    this->config=config;
    this->logger=logger;
}
void GwSocketServer::begin(){
    server=new WiFiServer(config->getInt(config->serverPort),config->getInt(config->maxClients));
    server->begin();
    logger->logString("Socket server created, port=%d",
        config->getInt(config->serverPort));
    MDNS.addService("_nmea-0183","_tcp",config->getInt(config->serverPort));    

}
void GwSocketServer::loop()
{
    WiFiClient client = server->available(); // listen for incoming clients

    if (client){
        logger->logString("new client connected from %s",
            client.remoteIP().toString().c_str());
        fcntl(client.fd(), F_SETFL, O_NONBLOCK);
        gwClientPtr newClient(new GwClient(wiFiClientPtr(new WiFiClient(client)),logger));    
        clients.push_back(newClient);
    }
    //sending
    for (auto it = clients.begin(); it != clients.end();it++){
        GwBuffer::WriteStatus rt=(*it)->write();
        if (rt == GwBuffer::ERROR){
            LOG_DEBUG(GwLog::ERROR,"write error on %s, closing",(*it)->client->remoteIP().toString().c_str());
            (*it)->client->stop();
        }
    }
    for (auto it = clients.begin(); it != clients.end();it++)
    {
        if ((*it) != NULL)
        {
            if (!(*it)->client->connected())
            {
                logger->logString("client disconnect");
                (*it)->client->stop();
                clients.erase(it);
            }
            else
            {
                while ((*it)->client->available())
                {
                    char c = (*it)->client->read();
                    //TODO: read data
                }
            }
        }
        else
        {
            it = clients.erase(it); // Should have been erased by StopClient
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
    for (auto it = clients.begin() ; it != clients.end(); it++) {
    if ( (*it) != NULL && (*it)->client->connected() ) {
        bool rt=(*it)->enqueue((uint8_t*)buffer,len);
        if (!rt){
            LOG_DEBUG(GwLog::DEBUG,"overflow in send to %s",(*it)->client->remoteIP().toString().c_str());    
        }
        
    }
  }
}

int GwSocketServer::numClients(){
    return clients.size();
}