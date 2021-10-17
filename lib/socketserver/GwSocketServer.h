#ifndef _GWSOCKETSERVER_H
#define _GWSOCKETSERVER_H
#include "GWConfig.h"
#include "GwLog.h"
#include <list>
#include <memory>
#include <WiFi.h>

using wiFiClientPtr = std::shared_ptr<WiFiClient>;
class GwSocketServer{
    private:
        const GwConfigHandler *config;
        GwLog *logger;
        std::list<wiFiClientPtr> clients;
        WiFiServer *server=NULL;
    public:
        GwSocketServer(const GwConfigHandler *config,GwLog *logger);
        void begin();
        void loop();
        void sendToClients(const char *buf);
        int numClients();
};
#endif