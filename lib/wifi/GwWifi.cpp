#include "GWWifi.h"

const char *AP_password = "esp32nmea2k"; 

GwWifi::GwWifi(const GwConfigHandler *config,GwLog *log, bool fixedApPass){
    this->config=config;
    this->logger=log;
    wifiClient=config->getConfigItem(config->wifiClient,true);
    wifiSSID=config->getConfigItem(config->wifiSSID,true);
    wifiPass=config->getConfigItem(config->wifiPass,true);
    this->fixedApPass=fixedApPass;
}
void GwWifi::setup(){
    LOG_DEBUG(GwLog::LOG,"Wifi setup");
    
    IPAddress AP_local_ip(192, 168, 15, 1);  // Static address for AP
    IPAddress AP_gateway(192, 168, 15, 1);
    IPAddress AP_subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_MODE_APSTA); //enable both AP and client
    const char *ssid=config->getConfigItem(config->systemName)->asCString();
    if (fixedApPass){
        WiFi.softAP(ssid,AP_password);
    }
    else{
        WiFi.softAP(ssid,config->getConfigItem(config->apPassword)->asCString());
    }
    delay(100);
    WiFi.softAPConfig(AP_local_ip, AP_gateway, AP_subnet);
    LOG_DEBUG(GwLog::LOG,"WifiAP created: ssid=%s,adress=%s",
        ssid,
        WiFi.softAPIP().toString().c_str()
        );
    apActive=true;   
    lastApAccess=millis();
    apShutdownTime=config->getConfigItem(config->stopApTime)->asInt() * 60;
    if (apShutdownTime < 120 && apShutdownTime != 0) apShutdownTime=120; //min 2 minutes
    LOG_DEBUG(GwLog::LOG,"GWWIFI: AP auto shutdown %s (%ds)",apShutdownTime> 0?"enabled":"disabled",apShutdownTime);
    apShutdownTime=apShutdownTime*1000; //ms   
    clientIsConnected=false;
    connectInternal();    
}
bool GwWifi::connectInternal(){
    if (wifiClient->asBoolean()){
        clientIsConnected=false;
        LOG_DEBUG(GwLog::LOG,"creating wifiClient ssid=%s",wifiSSID->asString().c_str());
        wl_status_t rt=WiFi.begin(wifiSSID->asCString(),wifiPass->asCString());
        LOG_DEBUG(GwLog::LOG,"wifiClient connect returns %d",(int)rt);
        lastConnectStart=millis();
        return true;
    }
    return false;
}
#define RETRY_MILLIS 20000
void GwWifi::loop(){
    if (wifiClient->asBoolean())
    {
        if (!clientConnected())
        {
            long now = millis();
            if (lastConnectStart > now || (lastConnectStart + RETRY_MILLIS) < now)
            {
                LOG_DEBUG(GwLog::LOG,"wifiClient: retry connect to %s", wifiSSID->asCString());
                WiFi.disconnect();
                connectInternal();
            }
        }
        else{
            if (! clientIsConnected){
                LOG_DEBUG(GwLog::LOG,"client %s now connected",wifiSSID->asCString());
                clientIsConnected=true;
            }
        }
    }
    if (apShutdownTime != 0 && apActive){
        if (WiFi.softAPgetStationNum()){
            lastApAccess=millis();
        }
        if ((lastApAccess + apShutdownTime) < millis()){
            LOG_DEBUG(GwLog::LOG,"GWWIFI: shutdown AP");
            WiFi.softAPdisconnect(true);
            apActive=false;
        }
    }
}
bool GwWifi::clientConnected(){
    return WiFi.status() == WL_CONNECTED;
};
bool GwWifi::connectClient(){
    WiFi.disconnect();
    return connectInternal();
}

String GwWifi::apIP(){
    if (! apActive) return String();
    return WiFi.softAPIP().toString();
}