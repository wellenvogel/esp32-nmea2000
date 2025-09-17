#ifndef _GWUDPREADER_H
#define _GWUDPREADER_H
#include "GWConfig.h"
#include "GwLog.h"
#include "GwBuffer.h"
#include "GwChannelInterface.h"
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>

class GwUdpReader: public GwChannelInterface{
    public:
    using UType=enum{
        T_ALL=0,
        T_AP=1,
        T_STA=2,
        T_MCALL=4,
        T_MCAP=5,
        T_MCSTA=6,
        T_UNKNOWN=-1
    };
    private:
        const GwConfigHandler *config;
        GwLog *logger;
        int minId;
        int port;
        int fd=-1;
        struct sockaddr_in listenA;
        String listenIp;
        String currentStationIp;
        struct in_addr apAddr;
        struct in_addr staAddr;
        UType type=T_UNKNOWN;
        void createAndBind();
        bool setStationAdd(const String &sta);
        GwBuffer *buffer=nullptr;
    public:
        GwUdpReader(const GwConfigHandler *config,GwLog *logger,int minId);
        ~GwUdpReader();
        void begin();
        virtual void loop(bool handleRead=true,bool handleWrite=true);
        virtual size_t sendToClients(const char *buf,int sourceId, bool partialWrite=false);
        virtual void readMessages(GwMessageFetcher *writer);
        virtual int getType() override;
};
#endif