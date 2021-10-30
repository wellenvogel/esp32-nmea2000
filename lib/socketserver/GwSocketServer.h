#ifndef _GWSOCKETSERVER_H
#define _GWSOCKETSERVER_H
#include "GWConfig.h"
#include "GwLog.h"
#include "GwBuffer.h"
#include <memory>
#include <WiFi.h>

using wiFiClientPtr = std::shared_ptr<WiFiClient>;
class GwClient;
using gwClientPtr = std::shared_ptr<GwClient>;
class GwSocketServer{
    private:
        const GwConfigHandler *config;
        GwLog *logger;
        gwClientPtr *clients=NULL;
        WiFiServer *server=NULL;
        bool allowReceive;
        int maxClients;
        int minId;
    public:
        GwSocketServer(const GwConfigHandler *config,GwLog *logger,int minId);
        ~GwSocketServer();
        void begin();
        void loop(bool handleRead=true);
        void sendToClients(const char *buf,int sourceId);
        int numClients();
        bool readMessages(GwBufferWriter *writer);
};
#endif