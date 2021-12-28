#pragma once
#include <Arduino.h>
#include <lwip/sockets.h>
#include "GwBuffer.h"
class GwSocketConnection
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
    static IPAddress remoteIP(int fd);
    GwSocketConnection(GwLog *logger, int id, bool allowRead = false);
    void setClient(int fd);
    bool hasClient();
    void stop();
    ~GwSocketConnection();
    bool connected();
    bool enqueue(uint8_t *data, size_t len);
    bool hasData();
    bool handleError(int res, bool errorIf0 = true);
    GwBuffer::WriteStatus write();
    bool read();
    bool messagesFromBuffer(GwMessageFetcher *writer);
};
