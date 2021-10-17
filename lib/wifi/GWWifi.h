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
    public:
        GwWifi(const GwConfigHandler *config,GwLog *log);
        void setup();
        void loop();
        bool clientConnected();
        bool connectClient();
};
#endif