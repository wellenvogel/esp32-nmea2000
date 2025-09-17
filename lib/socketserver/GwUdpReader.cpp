#include "GwUdpReader.h"
#include <ESPmDNS.h>
#include <errno.h>
#include "GwBuffer.h"
#include "GwSocketConnection.h"
#include "GwSocketHelper.h"
#include "GWWifi.h"
#include "GwHardware.h"


GwUdpReader::GwUdpReader(const GwConfigHandler *config, GwLog *logger, int minId)
{
    this->config = config;
    this->logger = logger;
    this->minId = minId;
    port=config->getInt(GwConfigDefinitions::udprPort);
    buffer= new GwBuffer(logger,GwBuffer::RX_BUFFER_SIZE,"udprd");
}

void GwUdpReader::createAndBind(){
    if (fd >= 0){
        ::close(fd);
    }
    if (currentStationIp.isEmpty() && (type == T_STA || type == T_MCSTA)) return;
    fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);
    if (fd < 0){
        LOG_ERROR("UDPR: unable to create udp socket: %d",errno);
        return;
    }
    int enable = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (type == T_STA)
    {
        if (inet_pton(AF_INET, currentStationIp.c_str(), &listenA.sin_addr) != 1)
        {
            LOG_ERROR("UDPR: invalid station ip address %s", currentStationIp.c_str());
            close(fd);
            fd = -1;
            return;
        }
    }
    if (bind(fd,(struct sockaddr *)&listenA,sizeof(listenA)) < 0){
        LOG_ERROR("UDPR: unable to bind: %d",errno);
        close(fd);
        fd=-1;
        return;
    }
    LOG_INFO("UDPR: socket created and bound");
    if (type != T_MCALL && type != T_MCAP && type != T_MCSTA) {
        return;
    }
    struct ip_mreq mc;
    mc.imr_multiaddr=listenA.sin_addr;
    if (type == T_MCALL || type == T_MCAP){
        mc.imr_interface=apAddr;
        int res=setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mc,sizeof(mc));
        if (res != 0){
            LOG_ERROR("UDPR: unable to add MC membership for AP:%d",errno);
        }
        else{
            LOG_INFO("UDPR: membership for for AP");
        }
    }
    if (!currentStationIp.isEmpty() && (type == T_MCALL || type == T_MCSTA))
    {
        mc.imr_interface = staAddr;
        int res = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mc, sizeof(mc));
        if (res != 0)
        {
            LOG_ERROR("UDPR: unable to add MC membership for STA:%d", errno);
        }
        else{
            LOG_INFO("UDPR: membership for STA %s",currentStationIp.c_str());
        }
    }
}

void GwUdpReader::begin()
{
    if (type != T_UNKNOWN) return; //already started
    type=(UType)(config->getInt(GwConfigDefinitions::udprType));
    LOG_INFO("UDPR begin, mode=%d",(int)type);
    port=config->getInt(GwConfigDefinitions::udprPort);
    listenA.sin_family=AF_INET;
    listenA.sin_port=htons(port);
    listenA.sin_addr.s_addr=htonl(INADDR_ANY); //default
    String ap=WiFi.softAPIP().toString();
    if (inet_pton(AF_INET, ap.c_str(), &apAddr) != 1)
    {
        LOG_ERROR("UDPR: invalid ap ip address %s", ap.c_str());
        return;
    }
    if (type == T_MCALL || type == T_MCAP || type == T_MCSTA){
        String mcAddr=config->getString(GwConfigDefinitions::udprMC);
        if (inet_pton(AF_INET, mcAddr.c_str(), &listenA.sin_addr) != 1)
        {
            LOG_ERROR("UDPR: invalid mc address %s", mcAddr.c_str());
            close(fd);
            fd = -1;
            return;
        }
        LOG_INFO("UDPR: using multicast address %s",mcAddr.c_str());
    }
    if (type == T_AP){
        listenA.sin_addr=apAddr;
    }
    String sta;
    if (WiFi.isConnected()) sta=WiFi.localIP().toString();
    setStationAdd(sta);
    createAndBind();
}

bool GwUdpReader::setStationAdd(const String &sta){
    if (sta == currentStationIp) return false;
    currentStationIp=sta;
    if (inet_pton(AF_INET, currentStationIp.c_str(), &staAddr) != 1){
            LOG_ERROR("UDPR: invalid station ip address %s", currentStationIp.c_str());
            return false;
    }
    LOG_INFO("UDPR: new station IP %s",currentStationIp.c_str());
    return true;
}
void GwUdpReader::loop(bool handleRead, bool handleWrite)
{
    if (handleRead){
        if (type == T_STA || type == T_MCALL || type == T_MCSTA){
            //only change anything if we considered the station IP
            String nextStationIp;
            if (WiFi.isConnected()){
                nextStationIp=WiFi.localIP().toString();
            }
            if (setStationAdd(nextStationIp)){
                LOG_INFO("UDPR: wifi client IP changed, restart");
                createAndBind();
            }
        }
    }
    
}

void GwUdpReader::readMessages(GwMessageFetcher *writer)
{
    if (fd < 0) return;
    //we expect one NMEA message in one UDP packet
    buffer->reset();
    size_t rd=buffer->fillData(buffer->freeSpace(),
    [this](uint8_t *rcvb,size_t rcvlen,void *param)->size_t{
        struct sockaddr_in from;
        socklen_t fromLen=sizeof(from);
        ssize_t res=recvfrom(fd,rcvb,rcvlen,MSG_DONTWAIT,
        (struct sockaddr*)&from,&fromLen);
        if (res <= 0) return 0;
        if (GwSocketHelper::equals(from.sin_addr,apAddr)) return 0;
        if (!currentStationIp.isEmpty() && (GwSocketHelper::equals(from.sin_addr,staAddr))) return 0;
        return res;
    },this);
    if (buffer->usedSpace() > 0)(GwLog::DEBUG,"UDPR: received %d bytes",buffer->usedSpace());
    writer->handleBuffer(buffer);   
}
size_t GwUdpReader::sendToClients(const char *buf, int source,bool partial)
{ 
    return 0;
}


GwUdpReader::~GwUdpReader()
{
}
int GwUdpReader::getType(){return GWSERIAL_TYPE_BI;}