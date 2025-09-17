#ifndef _GWSOCKETSERVER_H
#define _GWSOCKETSERVER_H
#include "GWConfig.h"
#include "GwLog.h"
#include "GwBuffer.h"
#include "GwChannelInterface.h"
#include <memory>

class GwSocketConnection;
class GwSocketServer: public GwChannelInterface{
    private:
        const GwConfigHandler *config;
        GwLog *logger;
        GwSocketConnection **clients=NULL;
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
        virtual void loop(bool handleRead=true,bool handleWrite=true);
        virtual size_t sendToClients(const char *buf,int sourceId, bool partialWrite=false);
        int numClients();
        virtual void readMessages(GwMessageFetcher *writer);
        virtual int getType() override;
};
#endif