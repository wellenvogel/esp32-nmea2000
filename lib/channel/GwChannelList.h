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
    class SerialWrapperBase{
        public:
        virtual void begin(GwLog* logger,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1)=0;
        virtual Stream *getStream()=0;
        virtual int getId()=0;
    };
        GwLog *logger;
        GwConfigHandler *config;
        typedef std::vector<GwChannel *> ChannelList;
        ChannelList theChannels;
        std::map<int,String> modes;
        GwSocketServer *sockets;
        GwTcpClient *client;
        void addSerial(SerialWrapperBase *stream,const String &mode,int rx,int tx);
        void addSerial(SerialWrapperBase *stream,int type,int rx,int tx);
    public:
        void addSerial(int id, int rx, int tx, int type);
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
