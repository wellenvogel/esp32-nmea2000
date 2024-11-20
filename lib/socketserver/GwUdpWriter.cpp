#include "GwUdpWriter.h"
#include <ESPmDNS.h>
#include <errno.h>
#include "GwBuffer.h"
#include "GwSocketConnection.h"
#include "GwSocketHelper.h"
#include "GWWifi.h"

GwUdpWriter::WriterSocket::WriterSocket(GwLog *l,int p,const String &src,const String &dst, SourceMode sm) :
     sourceMode(sm), source(src), destination(dst), port(p),logger(l)
{
    if (inet_pton(AF_INET, dst.c_str(), &dstA.sin_addr) != 1)
    {
        LOG_ERROR("UDPW: invalid destination ip address %s", dst.c_str());
        return;
    }
    if (sourceMode != SourceMode::S_UNBOUND)
    {
        if (inet_pton(AF_INET, src.c_str(), &srcA) != 1)
        {
            LOG_ERROR("UDPW: invalid source ip address %s", src.c_str());
            return;
        }
    }
    dstA.sin_family=AF_INET;
    dstA.sin_port=htons(port);
    fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);
    if (fd < 0){
        LOG_ERROR("UDPW: unable to create udp socket: %d",errno);
        return;
    }
    int enable = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(int));
    switch (sourceMode)
    {
    case SourceMode::S_SRC:
    {
        sockaddr_in bindA;
        bindA.sin_family = AF_INET;
        bindA.sin_port = htons(0); // let system select
        bindA.sin_addr = srcA;
        if (bind(fd, (struct sockaddr *)&bindA, sizeof(bindA)) != 0)
        {
            LOG_ERROR("UDPW: bind failed for address %s: %d", source.c_str(), errno);
            ::close(fd);
            fd = -1;
            return;
        }
    }
    break;
    case SourceMode::S_MC:
    {
        if (setsockopt(fd,IPPROTO_IP,IP_MULTICAST_IF,&srcA,sizeof(srcA)) != 0){
            LOG_ERROR("UDPW: unable to set MC source %s: %d",source.c_str(),errno);
            ::close(fd);
            fd=-1;
            return;
        }
        int loop=0;
        setsockopt(fd,IPPROTO_IP,IP_MULTICAST_LOOP,&loop,sizeof(loop));   
    }
    break;
    default:
      //not bound
      break;
    }
}
bool GwUdpWriter::WriterSocket::changed(const String &newSrc, const String &newDst){
    if (newDst != destination) return true;
    if (sourceMode == SourceMode::S_UNBOUND) return false;
    return newSrc != source;
}
size_t GwUdpWriter::WriterSocket::send(const char *buf,size_t len){
    if (fd < 0) return 0;
    ssize_t err = sendto(fd,buf,len,0,(struct sockaddr *)&dstA, sizeof(dstA));
    if (err < 0){
        LOG_DEBUG(GwLog::DEBUG,"UDPW %s error sending: %d",destination.c_str(), errno);
        return 0;
    }
    return err;
}

GwUdpWriter::GwUdpWriter(const GwConfigHandler *config, GwLog *logger, int minId)
{
    this->config = config;
    this->logger = logger;
    this->minId = minId;
    port=config->getInt(GwConfigDefinitions::udpwPort);
    
}
void GwUdpWriter::checkStaSocket(){
    String src;
    String bc;
    if (type == T_BCAP || type == T_MCAP || type == T_NORM || type == T_UNKNOWN ) return;
    bool connected=false;
    if (WiFi.isConnected()){
        src=WiFi.localIP().toString();
        bc=WiFi.broadcastIP().toString();
        connected=true;
    }
    else{
        if (staSocket == nullptr) return;
    }
    String dst;
    WriterSocket::SourceMode sm=WriterSocket::SourceMode::S_SRC;
    switch (type){
        case T_BCALL:
        case T_BCSTA:
            sm=WriterSocket::SourceMode::S_SRC;
            dst=bc;
            break;
        case T_MCALL:
        case T_MCSTA:
            dst=config->getString(GwConfigDefinitions::udpwMC);
            sm=WriterSocket::SourceMode::S_MC;
            break;

    }
    if (staSocket != nullptr)
    {
        if (!connected || staSocket->changed(src, dst))
        {
            staSocket->close();
            delete staSocket;
            staSocket = nullptr;
            LOG_INFO("changing/stopping UDPW(sta) socket");
        }
    }
    if (staSocket == nullptr && connected)
    {
        LOG_INFO("creating new UDP(sta) socket src=%s, dst=%s", src.c_str(), dst.c_str());
        staSocket = new WriterSocket(logger, port, src, dst, WriterSocket::SourceMode::S_SRC);
    }
}

void GwUdpWriter::begin()
{
    if (type != T_UNKNOWN) return; //already started
    type=(UType)(config->getInt(GwConfigDefinitions::udpwType));
    LOG_INFO("UDPW begin, mode=%d",(int)type);
    String src=WiFi.softAPIP().toString();
    String dst;
    WriterSocket::SourceMode sm=WriterSocket::SourceMode::S_UNBOUND;
    bool createApSocket=false;
    switch(type){
        case T_BCALL:
        case T_BCAP:
            createApSocket=true;
            dst=WiFi.softAPBroadcastIP().toString();   
            sm=WriterSocket::SourceMode::S_SRC;
            break;
        case T_MCALL:
        case T_MCAP:
            createApSocket=true;
            dst=config->getString(GwConfigDefinitions::udpwMC);   
            sm=WriterSocket::SourceMode::S_SRC;
            break;
        case T_NORM:
            createApSocket=true;
            dst=config->getString(GwConfigDefinitions::udpwAddress);   
            sm=WriterSocket::SourceMode::S_UNBOUND;
    }
    if (createApSocket){
        LOG_INFO("creating new UDPW(ap) socket src=%s, dst=%s", src.c_str(), dst.c_str());
        apSocket=new WriterSocket(logger,port,src,dst,sm);
    }
    checkStaSocket();
}

void GwUdpWriter::loop(bool handleRead, bool handleWrite)
{
    if (handleWrite){
        checkStaSocket();
    }
    
}

void GwUdpWriter::readMessages(GwMessageFetcher *writer)
{
    
}
size_t GwUdpWriter::sendToClients(const char *buf, int source,bool partial)
{ 
    if (source == minId) return 0;
    size_t len=strlen(buf);
    bool hasSent=false;
    size_t res=0;
    if (apSocket != nullptr){
        res=apSocket->send(buf,len);
        if (res > 0) hasSent=true;
    }
    if (staSocket != nullptr){
        res=staSocket->send(buf,len);
        if (res > 0) hasSent=true;
    }
    return hasSent?len:0;
}


GwUdpWriter::~GwUdpWriter()
{
}