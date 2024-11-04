#ifndef _GWUDPWRITER_H
#define _GWUDPWRITER_H
#include "GWConfig.h"
#include "GwLog.h"
#include "GwBuffer.h"
#include "GwChannelInterface.h"
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>

class GwUdpWriter: public GwChannelInterface{
    private:
        const GwConfigHandler *config;
        GwLog *logger;
        int fd=-1;
        int minId;
        int port;
        String address;
        struct sockaddr_in destination;
    public:
        GwUdpWriter(const GwConfigHandler *config,GwLog *logger,int minId);
        ~GwUdpWriter();
        void begin();
        virtual void loop(bool handleRead=true,bool handleWrite=true);
        virtual size_t sendToClients(const char *buf,int sourceId, bool partialWrite=false);
        virtual void readMessages(GwMessageFetcher *writer);
};
#endif