#include "GWWifi.h"

const char *AP_password = "esp32nmea2k"; 

GwWifi::GwWifi(const GwConfigHandler *config,GwLog *log){
    this->config=config;
    this->logger=log;
    wifiClient=config->getConfigItem(config->wifiClient,true);
    wifiSSID=config->getConfigItem(config->wifiSSID,true);
    wifiPass=config->getConfigItem(config->wifiPass,true);
}
void GwWifi::setup(){
    logger->logString("Wifi setup");
    
    IPAddress AP_local_ip(192, 168, 15, 1);  // Static address for AP
    IPAddress AP_gateway(192, 168, 15, 1);
    IPAddress AP_subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_MODE_APSTA); //enable both AP and client
    const char *ssid=config->getConfigItem(config->systemName)->asCString();
    WiFi.softAP(ssid,AP_password);
    delay(100);
    WiFi.softAPConfig(AP_local_ip, AP_gateway, AP_subnet);
    logger->logString("WifiAP created: ssid=%s,adress=%s",
        ssid,
        WiFi.softAPIP().toString().c_str()
        );
    apActive=true;   
    lastApAccess=millis();
    apShutdownTime=config->getConfigItem(config->stopApTime)->asInt() * 60;
    if (apShutdownTime < 120 && apShutdownTime != 0) apShutdownTime=120; //min 2 minutes
    logger->logString("GWWIFI: AP auto shutdown %s (%ds)",apShutdownTime> 0?"enabled":"disabled",apShutdownTime);
    apShutdownTime=apShutdownTime*1000; //ms   
    connectInternal();    
}
bool GwWifi::connectInternal(){
    if (wifiClient->asBoolean()){
        logger->logString("creating wifiClient ssid=%s",wifiSSID->asString().c_str());
        WiFi.begin(wifiSSID->asCString(),wifiPass->asCString());
        lastConnectStart=millis();
        return true;
    }
    return false;
}
#define RETRY_MILLIS 5000
void GwWifi::loop(){
    if (wifiClient->asBoolean() && ! clientConnected()){
        long now=millis();
        if (lastConnectStart > now || (lastConnectStart+RETRY_MILLIS) < now){
            logger->logString("wifiClient: retry connect to %s",wifiSSID->asCString());
            WiFi.disconnect();
            connectInternal();
        }
    }
    if (apShutdownTime != 0 && apActive){
        if (WiFi.softAPgetStationNum()){
            lastApAccess=millis();
        }
        if ((lastApAccess + apShutdownTime) < millis()){
            logger->logString("GWWIFI: shutdown AP");
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