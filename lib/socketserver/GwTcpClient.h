#pragma once
#include "GwSocketConnection.h"
#include "GwChannelInterface.h"
#include "GwSynchronized.h"
class GwTcpClient : public GwChannelInterface
{
    class ResolvedAddress{
        public:
            IPAddress address;
            bool resolved=false;
    };
    static const unsigned long CON_TIMEOUT=10000;
    GwSocketConnection *connection = NULL;
    String remoteAddress;
    ResolvedAddress resolvedAddress;
    uint16_t port = 0;
    unsigned long connectStart=0;
    GwLog *logger;
    int sourceId;
    bool configured=false;
    String error;
    SemaphoreHandle_t locker;

public:
    typedef enum
    {
        C_DISABLED = 0,
        C_INITIALIZED = 1,
        C_RESOLVING = 2,
        C_RESOLVED = 3,
        C_CONNECTING = 4,
        C_CONNECTED = 5
    } State;

private:

    State state = C_DISABLED;
    void stop();
    void startResolving();
    void startConnection();
    void checkConnection();
    bool hasConfig();
    void resolveHost(String host);
    void setResolved(IPAddress addr, bool valid);
    ResolvedAddress getResolved();

public:
    GwTcpClient(GwLog *logger);
    ~GwTcpClient();
    void begin(int sourceId,String address, uint16_t port,bool allowRead);
    virtual void loop(bool handleRead=true,bool handleWrite=true);
    virtual size_t sendToClients(const char *buf,int sourceId, bool partialWrite=false);
    virtual void readMessages(GwMessageFetcher *writer);
    bool isConnected();
    String getError(){return error;}
};