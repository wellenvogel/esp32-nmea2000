#pragma once
#include <functional>
#include <vector>
#include <WString.h>
#include "GwChannel.h"
#include "GwLog.h"
#include "GWConfig.h"
#include "GwJsonDocument.h"
#include "GwApi.h"

//NMEA message channels
#define N2K_CHANNEL_ID 0
#define USB_CHANNEL_ID 1
#define SERIAL1_CHANNEL_ID 2
#define TCP_CLIENT_CHANNEL_ID 3
#define MIN_TCP_CHANNEL_ID 4

#define MIN_USER_TASK 200
class GwSocketServer;
class GwTcpClient;
class GwChannelList{
    private:
        GwLog *logger;
        GwConfigHandler *config;
        typedef std::vector<GwChannel *> ChannelList;
        ChannelList theChannels;
        
        GwSocketServer *sockets;
        GwTcpClient *client;
        String serialMode=F("NONE");
    public:
        GwChannelList(GwLog *logger, GwConfigHandler *config);
        typedef std::function<void(GwChannel *)> ChannelAction;
        void allChannels(ChannelAction action);
        //initialize
        void begin(bool fallbackSerial=false);
        //status
        int getJsonSize();
        void toJson(GwJsonDocument &doc);
        //single channel
        GwChannel *getChannelById(int sourceId);
        void fillStatus(GwApi::Status &status);


};
