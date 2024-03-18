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
        std::map<int,String> modes;
        GwSocketServer *sockets;
        GwTcpClient *client;
        void addSerial(HardwareSerial *stream,int id,const String &mode,int rx,int tx);
        void addSerial(HardwareSerial *stream,int id,int type,int rx,int tx);
    public:
        static constexpr const char* serial="serial";
        static constexpr const char* serial2="serial2";
        void addSerial(const String &name, int rx, int tx, int type);
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
        String getMode(int id);

};
