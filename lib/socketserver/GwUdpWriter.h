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
    public:
    using UType=enum{
        T_BCALL=0,
        T_BCAP=1,
        T_BCSTA=2,
        T_NORM=3,
        T_MCALL=4,
        T_MCAP=5,
        T_MCSTA=6,
        T_UNKNOWN=-1
    };
    private:
        class WriterSocket{
            public:
            int fd=-1;
            struct in_addr srcA;
            struct sockaddr_in dstA;
            String source;
            String destination;
            int port;
            GwLog *logger;
            using SourceMode=enum {
                S_UNBOUND=0,
                S_MC,
                S_SRC
            };
            SourceMode sourceMode;
            WriterSocket(GwLog *logger,int p,const String &src,const String &dst, SourceMode sm);
            void close(){
                if (fd > 0){
                    ::close(fd);
                }
                fd=-1;
            }
            ~WriterSocket(){
                close();
            }
            bool changed(const String &newSrc, const String &newDst);
            size_t send(const char *buf,size_t len);
        };
        const GwConfigHandler *config;
        GwLog *logger;
        /**
         * we use fd/address to send to the AP network
         * and fd2,address2 to send to the station network
         * for type "normal" we only use fd
        */
        WriterSocket *apSocket=nullptr; //also for T_NORM
        WriterSocket *staSocket=nullptr;
        int minId;
        int port;
        UType type=T_UNKNOWN;
        void checkStaSocket();
    public:
        GwUdpWriter(const GwConfigHandler *config,GwLog *logger,int minId);
        ~GwUdpWriter();
        void begin();
        virtual void loop(bool handleRead=true,bool handleWrite=true);
        virtual size_t sendToClients(const char *buf,int sourceId, bool partialWrite=false);
        virtual void readMessages(GwMessageFetcher *writer);
        virtual int getType() override;
};
#endif