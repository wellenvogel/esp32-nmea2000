#pragma once
#include <functional>
#include <vector>
#include <map>
#include <WString.h>
#include "GwChannel.h"
#include "GwLog.h"
#include "GWConfig.h"
#include "GwJsonDocument.h"
#include "GwApi.h"
#include "GwSerial.h"
#include <HardwareSerial.h>

//NMEA message channels
#define N2K_CHANNEL_ID 0
#define USB_CHANNEL_ID 1
#define SERIAL1_CHANNEL_ID 2
#define SERIAL2_CHANNEL_ID 3
#define TCP_CLIENT_CHANNEL_ID 4
#define MIN_TCP_CHANNEL_ID 5

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
    public:
        void addChannel(GwChannel *);
        GwChannelList(GwLog *logger, GwConfigHandler *config);
        typedef std::function<void(GwChannel *)> ChannelAction;
        void allChannels(ChannelAction action);
        //called to allow setting some predefined configs
        //e.g. from serial definitions
        void preinit();
        //initialize
        void begin(bool fallbackSerial=false);
        //status
        int getJsonSize();
        void toJson(GwJsonDocument &doc);
        //single channel
        GwChannel *getChannelById(int sourceId);
        void fillStatus(GwApi::Status &status);
        String getMode(int id);

};
