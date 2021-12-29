#include "GwTcpClient.h"

bool GwTcpClient::hasConfig(){
    return configured;
}
bool GwTcpClient::isConnected(){
    return state == C_CONNECTED;
}
void GwTcpClient::stop()
{
    if (connection->hasClient())
    {
        LOG_DEBUG(GwLog::DEBUG, "stopping tcp client");
        connection->stop();
    }
    state = C_DISABLED;
}
void GwTcpClient::startConnection()
{
    //TODO
    state = C_INITIALIZED;
    connectStart=millis();
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG_DEBUG(GwLog::ERROR,"unable to create socket: %d", errno);
        return; 
    }
    fcntl( sockfd, F_SETFL, fcntl( sockfd, F_GETFL, 0 ) | O_NONBLOCK );

    uint32_t ip_addr = this->remoteAddress;
    struct sockaddr_in serveraddr;
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    memcpy((void *)&serveraddr.sin_addr.s_addr, (const void *)(&ip_addr), 4);
    serveraddr.sin_port = htons(port);
    int res = lwip_connect_r(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (res < 0 ) {
        if (errno != EINPROGRESS){
            LOG_DEBUG(GwLog::ERROR,"connect on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
            close(sockfd);
            return;
        }
        state=C_CONNECTING;
        connection->setClient(sockfd);
    }
    else{
        state=C_CONNECTED;
        connection->setClient(sockfd);
    }
}
void GwTcpClient::checkConnection()
{
    unsigned long now=millis();
    if (! connection->hasClient()){
        state = hasConfig()?C_INITIALIZED:C_DISABLED;
    }
    if (state == C_INITIALIZED){
        if ((now - connectStart) > CON_TIMEOUT){
            LOG_DEBUG(GwLog::LOG,"retry connect to %s",remoteAddress.toString().c_str());
            startConnection();
        }
        return;
    }
    if (state != C_CONNECTING){
        return;
    }
    fd_set fdset;
    struct timeval tv;
    FD_ZERO(&fdset);
    int sockfd=connection->fd;
    FD_SET(connection->fd, &fdset);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int res = select(sockfd + 1, nullptr, &fdset, nullptr, &tv);
    if (res < 0) {
        LOG_DEBUG(GwLog::ERROR,"select on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
        connection->stop();
        return;
    } else if (res == 0) {
        //still connecting
        if ((now - connectStart) >= CON_TIMEOUT){
            LOG_DEBUG(GwLog::ERROR,"connect timeout to %s, retry",remoteAddress.toString().c_str());
            connection->stop();
            return;
        }
    } else {
        int sockerr;
        socklen_t len = (socklen_t)sizeof(int);
        res = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);

        if (res < 0) {
            LOG_DEBUG(GwLog::ERROR,"getsockopt on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
            connection->stop();
            return;
        }
        if (sockerr != 0) {
            LOG_DEBUG(GwLog::ERROR,"socket error on fd %d, errno: %d, \"%s\"", sockfd, sockerr, strerror(sockerr));
            connection->stop();
            return;
        }
    }
    LOG_DEBUG(GwLog::LOG,"connected to %s",remoteAddress.toString().c_str());
    state=C_CONNECTED;
}

GwTcpClient::GwTcpClient(GwLog *logger)
{
    this->logger = logger;
}
GwTcpClient::~GwTcpClient(){
    if (connection)
    delete connection;
}
void GwTcpClient::begin(int sourceId,IPAddress address, uint16_t port,bool allowRead)
{
    stop();
    this->sourceId=sourceId;
    this->remoteAddress = address;
    this->port = port;
    configured=true;
    state = C_INITIALIZED;
    this->connection = new GwSocketConnection(logger,0,allowRead);
    startConnection();
}
void GwTcpClient::loop(bool handleRead,bool handleWrite)
{
    checkConnection();
    if (state != C_CONNECTED){
        return;
    }
    if (handleRead){
        if (connection->hasClient()){
            if (! connection->connected()){
                LOG_DEBUG(GwLog::ERROR,"connection closed on %s",connection->remoteIpAddress.c_str());
                connection->stop();
            }
            else{
                connection->read();
            }  
        }
    }
    if (handleWrite){
        if (connection->hasClient()){
            GwBuffer::WriteStatus rt = connection->write();
            if (rt == GwBuffer::ERROR)
            {
                LOG_DEBUG(GwLog::ERROR, "write error on %s, closing", connection->remoteIpAddress.c_str());
                connection->stop();
            }
        }
    }
}

void GwTcpClient::sendToClients(const char *buf,int sourceId){
    if (sourceId == this->sourceId) return;
    if (state != C_CONNECTED) return;
    if (! connection->hasClient()) return;
    connection->enqueue((uint8_t*)buf,strlen(buf));
}
bool GwTcpClient::readMessages(GwMessageFetcher *writer){
    if (state != C_CONNECTED) return false;
    if (! connection->hasClient()) return false;
    return connection->messagesFromBuffer(writer);
}