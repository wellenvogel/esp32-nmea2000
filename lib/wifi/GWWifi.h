#ifndef _GWWIFI_H
#define _GWWIFI_H
#include <WiFi.h>
#include <GWConfig.h>
class GwWifi{
    private:
        const GwConfigHandler *config;
        GwLog *logger;
        const GwConfigInterface *wifiClient;
        const GwConfigInterface *wifiSSID;
        const GwConfigInterface *wifiPass;
        bool connectInternal();
        long lastConnectStart=0;
        unsigned long lastApAccess=0;
        unsigned long apShutdownTime=0;
        bool apActive=false;
        bool fixedApPass=true;
        bool clientIsConnected=false;
    public:
        const char *AP_password = "esp32nmea2k"; 
        GwWifi(const GwConfigHandler *config,GwLog *log, bool fixedApPass=true);
        void setup();
        void loop();
        bool clientConnected();
        bool connectClient();
        String apIP();
        bool isApActive(){return apActive;}
        bool isClientActive(){return wifiClient->asBoolean();}
};
#endif