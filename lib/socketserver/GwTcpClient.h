#pragma once
#include "GwSocketConnection.h"
#include "GwChannelInterface.h"
class GwTcpClient : public GwChannelInterface
{
    static const unsigned long CON_TIMEOUT=10000;
    GwSocketConnection *connection = NULL;
    String remoteAddress;
    uint16_t port = 0;
    unsigned long connectStart=0;
    GwLog *logger;
    int sourceId;
    bool configured=false;
    String error;

public:
    typedef enum
    {
        C_DISABLED = 0,
        C_INITIALIZED = 1,
        C_CONNECTING = 2,
        C_CONNECTED = 3
    } State;

private:
    State state = C_DISABLED;
    void stop();
    void startConnection();
    void checkConnection();
    bool hasConfig();

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