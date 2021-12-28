#include "GwSocketServer.h"
#include <ESPmDNS.h>
#include <lwip/sockets.h>
#include "GwBuffer.h"

class GwClient
{
public:
    int fd=-1;
    int overflows;
    String remoteIpAddress;

private:
    unsigned long lastWrite = 0;
    unsigned long writeTimeout = 10000;
    bool pendingWrite = false;
    bool writeError = false;
    bool allowRead;
    GwBuffer *buffer = NULL;
    GwBuffer *readBuffer = NULL;
    GwLog *logger;

public:
    static IPAddress remoteIP(int fd)
    {
        struct sockaddr_storage addr;
        socklen_t len = sizeof addr;
        getpeername(fd, (struct sockaddr*)&addr, &len);
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        return IPAddress((uint32_t)(s->sin_addr.s_addr));
    }
    GwClient(GwLog *logger, int id, bool allowRead = false)
    {
        this->logger = logger;
        this->allowRead = allowRead;
        String bufName = "Sock(";
        bufName += String(id);
        bufName += ")";
        buffer = new GwBuffer(logger, GwBuffer::TX_BUFFER_SIZE, bufName + "wr");
        if (allowRead)
        {
            readBuffer = new GwBuffer(logger, GwBuffer::RX_BUFFER_SIZE, bufName + "rd");
        }
        overflows = 0;
    }
    void setClient(int fd)
    {
        this->fd = fd;
        buffer->reset("new client");
        if (readBuffer)
            readBuffer->reset("new client");
        overflows = 0;
        pendingWrite = false;
        writeError = false;
        lastWrite = 0;
        if (fd >= 0)
        {
            remoteIpAddress = remoteIP(fd).toString();
        }
        else
        {
            remoteIpAddress = String("---");
        }
    }
    bool hasClient()
    {
        return fd >= 0;
    }
    void stop(){
        if (fd >= 0){
            close(fd);
            fd=-1;
        }
    }
    ~GwClient()
    {
        delete buffer;
        if (readBuffer)
            delete readBuffer;
    }
    bool connected()
    {
        if (fd >= 0)
        {
            uint8_t dummy;
            int res = recv(fd, &dummy, 0, MSG_DONTWAIT);
            // avoid unused var warning by gcc
            (void)res;
            // recv only sets errno if res is <= 0
            if (res <= 0)
            {
                switch (errno)
                {
                case EWOULDBLOCK:
                case ENOENT: //caused by vfs
                    return true;
                    break;
                case ENOTCONN:
                case EPIPE:
                case ECONNRESET:
                case ECONNREFUSED:
                case ECONNABORTED:
                    return false;
                    break;
                default:
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
        return false;
    }

    bool enqueue(uint8_t *data, size_t len)
    {
        if (len == 0)
            return true;
        size_t rt = buffer->addData(data, len);
        if (rt < len)
        {
            LOG_DEBUG(GwLog::LOG, "overflow on %s", remoteIpAddress.c_str());
            overflows++;
            return false;
        }
        return true;
    }
    bool hasData()
    {
        return buffer->usedSpace() > 0;
    }
    bool handleError(int res, bool errorIf0 = true)
    {
        if (res == 0 && errorIf0)
        {
            LOG_DEBUG(GwLog::LOG, "client shutdown (recv 0) on %s", remoteIpAddress.c_str());
            stop();
            return false;
        }
        if (res < 0)
        {
            if (errno != EAGAIN)
            {
                LOG_DEBUG(GwLog::LOG, "client read error %d on %s", errno, remoteIpAddress.c_str());
                stop();
                return false;
            }
            return false;
        }
        return true;
    }
    GwBuffer::WriteStatus write()
    {
        if (!hasClient())
        {
            LOG_DEBUG(GwLog::LOG, "write called on empty client");
            return GwBuffer::ERROR;
        }
        if (!buffer->usedSpace())
        {
            pendingWrite = false;
            return GwBuffer::OK;
        }
        buffer->fetchData(
            -1, [](uint8_t *buffer, size_t len, void *param) -> size_t
            {
                GwClient *c = (GwClient *)param;
                int res = send(c->fd, (void *)buffer, len, MSG_DONTWAIT);
                if (!c->handleError(res, false))
                    return 0;
                if (res >= len)
                {
                    c->pendingWrite = false;
                }
                else
                {
                    if (!c->pendingWrite)
                    {
                        c->lastWrite = millis();
                        c->pendingWrite = true;
                    }
                    else
                    {
                        //we need to check if we have still not been able
                        //to write until timeout
                        if (millis() >= (c->lastWrite + c->writeTimeout))
                        {
                            c->logger->logDebug(GwLog::ERROR, "Write timeout on channel %s", c->remoteIpAddress.c_str());
                            c->writeError = true;
                        }
                    }
                }
                return res;
            },
            this);
        if (writeError)
        {
            LOG_DEBUG(GwLog::DEBUG + 1, "write error on %s", remoteIpAddress.c_str());
            return GwBuffer::ERROR;
        }

        return GwBuffer::OK;
    }

    bool read()
    {
        if (!allowRead)
        {
            size_t maxLen = 100;
            char buffer[maxLen];
            int res = recv(fd, (void *)buffer, maxLen, MSG_DONTWAIT);
            return handleError(res);
        }
        readBuffer->fillData(
            -1, [](uint8_t *buffer, size_t len, void *param) -> size_t
            {
                GwClient *c = (GwClient *)param;
                int res = recv(c->fd, (void *)buffer, len, MSG_DONTWAIT);
                if (!c->handleError(res))
                    return 0;
                return res;
            },
            this);
        return true;
    }
    bool messagesFromBuffer(GwMessageFetcher *writer)
    {
        if (!allowRead)
            return false;
        return writer->handleBuffer(readBuffer);
    }
};

class GwTcpClient
{
    GwClient *gwClient = NULL;
    IPAddress remoteAddress;
    uint16_t port = 0;
    GwLog *logger;

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
    void stop()
    {
        if (gwClient->hasClient())
        {
            LOG_DEBUG(GwLog::DEBUG, "stopping tcp client");
            gwClient->stop();
        }
        state = C_DISABLED;
    }
    void startConnection()
    {
        //TODO
        state = C_CONNECTING;
    }
    void checkConnection()
    {
    }

public:
    GwTcpClient(GwLog *logger, GwClient *gwClient)
    {
        this->logger = logger;
        this->gwClient = gwClient;
    }
    void begin(IPAddress address, uint16_t port)
    {
        stop();
        this->remoteAddress = address;
        this->port = port;
        state = C_INITIALIZED;
        startConnection();
    }
    void loop()
    {
        if (state == C_CONNECTING)
        {
            checkConnection();
        }
    }
};

GwSocketServer::GwSocketServer(const GwConfigHandler *config, GwLog *logger, int minId)
{
    this->config = config;
    this->logger = logger;
    this->minId = minId;
    maxClients = 1;
    allowReceive = false;
}
bool GwSocketServer::createListener()
{
    struct sockaddr_in server;
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        return false;
    }
    int enable = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(listenerPort);
    if (bind(listener, (struct sockaddr *)&server, sizeof(server)) < 0)
        return false;
    if (listen(listener, maxClients) < 0)
        return false;
    fcntl(listener, F_SETFL, O_NONBLOCK);
    return true;
}
void GwSocketServer::begin()
{
    maxClients = config->getInt(config->maxClients);
    allowReceive = config->getBool(config->readTCP);
    listenerPort=config->getInt(config->serverPort);
    clients = new GwClient*[maxClients];
    for (int i = 0; i < maxClients; i++)
    {
        clients[i] = new GwClient(logger, i, allowReceive);
    }
    if (! createListener()){
        listener=-1;
        LOG_DEBUG(GwLog::ERROR,"Unable to create listener");
        return;
    }
    LOG_DEBUG(GwLog::LOG, "Socket server created, port=%d",
              config->getInt(config->serverPort));
    MDNS.addService("_nmea-0183", "_tcp", config->getInt(config->serverPort));
}
int GwSocketServer::available()
{
    if (listener < 0)
        return -1;
    int client_sock;
    struct sockaddr_in _client;
    int cs = sizeof(struct sockaddr_in);
    client_sock = lwip_accept_r(listener, (struct sockaddr *)&_client, (socklen_t *)&cs);
    if (client_sock >= 0)
    {
        int val = 1;
        if (setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&val, sizeof(int)) == ESP_OK)
        {
            if (setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(int)) == ESP_OK)
                fcntl(client_sock, F_SETFL, O_NONBLOCK);
                return client_sock;
        }
        close(client_sock);
    }
    return -1;
}
void GwSocketServer::loop(bool handleRead, bool handleWrite)
{
    if (!clients)
        return;
    int client = available(); // listen for incoming clients
    if (client >= 0)
    {
        LOG_DEBUG(GwLog::LOG, "new client connected from %s",
                  GwClient::remoteIP(client).toString().c_str());
        bool canHandle = false;
        for (int i = 0; i < maxClients; i++)
        {
            if (!clients[i]->hasClient())
            {
                clients[i]->setClient(client);
                LOG_DEBUG(GwLog::LOG, "set client as number %d", i);
                canHandle = true;
                break;
            }
        }
        if (!canHandle)
        {
            logger->logDebug(GwLog::ERROR, "no space to store client, disconnect");
            close(client);
        }
    }
    if (handleWrite)
    {
        //sending
        for (int i = 0; i < maxClients; i++)
        {
            GwClient *client = clients[i];
            if (!client->hasClient())
                continue;
            GwBuffer::WriteStatus rt = client->write();
            if (rt == GwBuffer::ERROR)
            {
                LOG_DEBUG(GwLog::ERROR, "write error on %s, closing", client->remoteIpAddress.c_str());
                client->stop();
            }
        }
    }
    for (int i = 0; i < maxClients; i++)
    {
        GwClient *client = clients[i];
        if (!client->hasClient())
            continue;

        if (!client->connected())
        {
            LOG_DEBUG(GwLog::LOG, "client %d disconnect %s", i, client->remoteIpAddress.c_str());
            client->stop();
        }
        else
        {
            if (handleRead)
                client->read();
        }
    }
}

bool GwSocketServer::readMessages(GwMessageFetcher *writer)
{
    if (!allowReceive || !clients)
        return false;
    bool hasMessages = false;
    for (int i = 0; i < maxClients; i++)
    {
        writer->id = minId + i;
        if (!clients[i]->hasClient())
            continue;
        if (clients[i]->messagesFromBuffer(writer))
            hasMessages = true;
    }
    return hasMessages;
}
void GwSocketServer::sendToClients(const char *buf, int source)
{
    if (!clients)
        return;
    int len = strlen(buf);
    int sourceIndex = source - minId;
    for (int i = 0; i < maxClients; i++)
    {
        if (i == sourceIndex)
            continue; //never send out to the source we received from
        GwClient *client = clients[i];
        if (!client->hasClient())
            continue;
        if (client->connected())
        {
            client->enqueue((uint8_t *)buf, len);
        }
    }
}

int GwSocketServer::numClients()
{
    if (!clients)
        return 0;
    int num = 0;
    for (int i = 0; i < maxClients; i++)
    {
        if (clients[i]->hasClient())
            num++;
    }
    return num;
}
GwSocketServer::~GwSocketServer()
{
}