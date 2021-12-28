#include "GwSocketConnection.h"
IPAddress GwSocketConnection::remoteIP(int fd)
{
    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;
    getpeername(fd, (struct sockaddr *)&addr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    return IPAddress((uint32_t)(s->sin_addr.s_addr));
}
GwSocketConnection::GwSocketConnection(GwLog *logger, int id, bool allowRead)
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
void GwSocketConnection::setClient(int fd)
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
bool GwSocketConnection::hasClient()
{
    return fd >= 0;
}
void GwSocketConnection::stop()
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}
GwSocketConnection::~GwSocketConnection()
{
    delete buffer;
    if (readBuffer)
        delete readBuffer;
}
bool GwSocketConnection::connected()
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

bool GwSocketConnection::enqueue(uint8_t *data, size_t len)
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
bool GwSocketConnection::hasData()
{
    return buffer->usedSpace() > 0;
}
bool GwSocketConnection::handleError(int res, bool errorIf0)
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
GwBuffer::WriteStatus GwSocketConnection::write()
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
            GwSocketConnection *c = (GwSocketConnection *)param;
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

bool GwSocketConnection::read()
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
            GwSocketConnection *c = (GwSocketConnection *)param;
            int res = recv(c->fd, (void *)buffer, len, MSG_DONTWAIT);
            if (!c->handleError(res))
                return 0;
            return res;
        },
        this);
    return true;
}
bool GwSocketConnection::messagesFromBuffer(GwMessageFetcher *writer)
{
    if (!allowRead)
        return false;
    return writer->handleBuffer(readBuffer);
}

