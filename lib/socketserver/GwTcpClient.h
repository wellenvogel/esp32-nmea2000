#pragma once
#include "GwSocketConnection.h"
class GwTcpClient
{
    static const unsigned long CON_TIMEOUT=10;
    GwSocketConnection *connection = NULL;
    IPAddress remoteAddress;
    uint16_t port = 0;
    unsigned long connectStart=0;
    GwLog *logger;
    int sourceId;
    bool configured=false;

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
    void begin(int sourceId,IPAddress address, uint16_t port,bool allowRead);
    void loop(bool handleRead=true,bool handleWrite=true);
    void sendToClients(const char *buf,int sourceId);
    bool readMessages(GwMessageFetcher *writer);
    bool isConnected();
};