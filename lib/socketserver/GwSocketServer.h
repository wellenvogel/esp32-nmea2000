#ifndef _GWSOCKETSERVER_H
#define _GWSOCKETSERVER_H
#include "GWConfig.h"
#include "GwLog.h"
#include "GwBuffer.h"
#include <memory>
#include <WiFi.h>

class GwClient;
class GwSocketServer{
    private:
        const GwConfigHandler *config;
        GwLog *logger;
        GwClient **clients=NULL;
        int listener=-1;
        int listenerPort=-1;
        bool allowReceive;
        int maxClients;
        int minId;
        bool createListener();
        int available();
    public:
        GwSocketServer(const GwConfigHandler *config,GwLog *logger,int minId);
        ~GwSocketServer();
        void begin();
        void loop(bool handleRead=true,bool handleWrite=true);
        void sendToClients(const char *buf,int sourceId);
        int numClients();
        bool readMessages(GwMessageFetcher *writer);
};
#endif