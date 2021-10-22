#include "GwSocketServer.h"
#include <ESPmDNS.h>

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
        clients.push_back(wiFiClientPtr(new WiFiClient(client)));
    }
    for (auto it = clients.begin(); it != clients.end();it++)
    {
        if ((*it) != NULL)
        {
            if (!(*it)->connected())
            {
                logger->logString("client disconnect ");
                (*it)->stop();
                clients.erase(it);
            }
            else
            {
                while ((*it)->available())
                {
                    char c = (*it)->read();
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
    for (auto it = clients.begin() ; it != clients.end(); it++) {
    if ( (*it) != NULL && (*it)->connected() ) {
      (*it)->println(buf);
    }
  }
}

int GwSocketServer::numClients(){
    return clients.size();
}