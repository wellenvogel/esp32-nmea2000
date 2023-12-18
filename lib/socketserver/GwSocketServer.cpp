#include "GwSocketServer.h"
#include <ESPmDNS.h>
#include <lwip/sockets.h>
#include "GwBuffer.h"
#include "GwSocketConnection.h"
#include "GwSocketHelper.h"

GwSocketServer::GwSocketServer(const GwConfigHandler *config, GwLog *logger, int minId)
{
    this->config = config;
    this->logger = logger;
    this->minId = minId;
    maxClients = 1;
    allowReceive = false;
}
bool GwSocketServer::createListener()
{
    struct sockaddr_in server;
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        return false;
    }
    int enable = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(listenerPort);
    if (bind(listener, (struct sockaddr *)&server, sizeof(server)) < 0)
        return false;
    if (listen(listener, maxClients) < 0)
        return false;
    fcntl(listener, F_SETFL, O_NONBLOCK);
    return true;
}
void GwSocketServer::begin()
{
    maxClients = config->getInt(config->maxClients);
    allowReceive = config->getBool(config->readTCP);
    listenerPort=config->getInt(config->serverPort);
    clients = new GwSocketConnection*[maxClients];
    for (int i = 0; i < maxClients; i++)
    {
        clients[i] = new GwSocketConnection(logger, i, allowReceive);
    }
    if (! createListener()){
        listener=-1;
        LOG_DEBUG(GwLog::ERROR,"Unable to create listener");
        return;
    }
    LOG_DEBUG(GwLog::LOG, "Socket server created, port=%d",
              config->getInt(config->serverPort));
    MDNS.addService("_nmea-0183", "_tcp", config->getInt(config->serverPort));
}
int GwSocketServer::available()
{
    if (listener < 0)
        return -1;
    int client_sock;
    struct sockaddr_in _client;
    int cs = sizeof(struct sockaddr_in);
    client_sock = accept(listener, (struct sockaddr *)&_client, (socklen_t *)&cs);
    if (client_sock >= 0)
    {
        int val = 1;
        if (! GwSocketHelper::setKeepAlive(client_sock,true)){
            LOG_DEBUG(GwLog::ERROR,"unable to set keepalive, nodelay on socket");
        }
        else
        {
            fcntl(client_sock, F_SETFL, O_NONBLOCK);
            return client_sock;
        }
        close(client_sock);
    }
    return -1;
}
void GwSocketServer::loop(bool handleRead, bool handleWrite)
{
    if (!clients)
        return;
    int client = available(); // listen for incoming clients
    if (client >= 0)
    {
        LOG_DEBUG(GwLog::LOG, "new client connected from %s",
                  GwSocketConnection::remoteIP(client).toString().c_str());
        bool canHandle = false;
        for (int i = 0; i < maxClients; i++)
        {
            if (!clients[i]->hasClient())
            {
                clients[i]->setClient(client);
                LOG_DEBUG(GwLog::LOG, "set client as number %d", i);
                canHandle = true;
                break;
            }
        }
        if (!canHandle)
        {
            logger->logDebug(GwLog::ERROR, "no space to store client, disconnect");
            close(client);
        }
    }
    if (handleWrite)
    {
        //sending
        for (int i = 0; i < maxClients; i++)
        {
            GwSocketConnection *client = clients[i];
            if (!client->hasClient())
                continue;
            GwBuffer::WriteStatus rt = client->write();
            if (rt == GwBuffer::ERROR)
            {
                LOG_DEBUG(GwLog::ERROR, "write error on %s, closing", client->remoteIpAddress.c_str());
                client->stop();
            }
        }
    }
    for (int i = 0; i < maxClients; i++)
    {
        GwSocketConnection *client = clients[i];
        if (!client->hasClient())
            continue;

        if (!client->connected())
        {
            LOG_DEBUG(GwLog::LOG, "client %d disconnect %s", i, client->remoteIpAddress.c_str());
            client->stop();
        }
        else
        {
            if (handleRead)
                client->read();
        }
    }
}

void GwSocketServer::readMessages(GwMessageFetcher *writer)
{
    if (!allowReceive || !clients)
        return;
    for (int i = 0; i < maxClients; i++)
    {
        writer->id = minId + i;
        if (!clients[i]->hasClient())
            continue;
        clients[i]->messagesFromBuffer(writer);
    }
    return;
}
size_t GwSocketServer::sendToClients(const char *buf, int source,bool partial)
{
    if (!clients)
        return 0;
    bool hasSend=false;    
    int len = strlen(buf);
    int sourceIndex = source - minId;
    for (int i = 0; i < maxClients; i++)
    {
        if (i == sourceIndex)
            continue; //never send out to the source we received from
        GwSocketConnection *client = clients[i];
        if (!client->hasClient())
            continue;
        if (client->connected())
        {
            if(client->enqueue((uint8_t *)buf, len)) hasSend=true;
        }
    }
    return hasSend?len:0;
}

int GwSocketServer::numClients()
{
    if (!clients)
        return 0;
    int num = 0;
    for (int i = 0; i < maxClients; i++)
    {
        if (clients[i]->hasClient())
            num++;
    }
    return num;
}
GwSocketServer::~GwSocketServer()
{
}