#include "GwUdpWriter.h"
#include <ESPmDNS.h>
#include <errno.h>
#include "GwBuffer.h"
#include "GwSocketConnection.h"
#include "GwSocketHelper.h"

GwUdpWriter::GwUdpWriter(const GwConfigHandler *config, GwLog *logger, int minId)
{
    this->config = config;
    this->logger = logger;
    this->minId = minId;
}

void GwUdpWriter::begin()
{
    fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);
    if (fd < 0){
        LOG_ERROR("unable to create udp socket");
        return;
    }
    int enable = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(int));
    port=config->getInt(GwConfigDefinitions::udpwPort);
    //TODO: check port
    address=config->getString(GwConfigDefinitions::udpwAddress);
    LOG_INFO("UDP writer created, address=%s, port=%d",
            address.c_str(),port);
    inet_pton(AF_INET, address.c_str(), &destination.sin_addr);
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
}

void GwUdpWriter::loop(bool handleRead, bool handleWrite)
{
    
}

void GwUdpWriter::readMessages(GwMessageFetcher *writer)
{
    
}
size_t GwUdpWriter::sendToClients(const char *buf, int source,bool partial)
{ 
    if (source == minId) return 0;
    size_t len=strlen(buf);
    ssize_t err = sendto(fd,buf,len,0,(struct sockaddr *)&destination, sizeof(destination));
    if (err < 0){
        LOG_DEBUG(GwLog::DEBUG,"UDP writer error sending: %d",errno);
        return 0;
    }
    return err;
}


GwUdpWriter::~GwUdpWriter()
{
}