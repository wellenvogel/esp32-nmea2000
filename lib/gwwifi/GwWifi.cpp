#include <esp_wifi.h>
#include "GWWifi.h"


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
    IPAddress defaultAddr(192,168,15,1);
    IPAddress AP_local_ip;  // Static address for AP
    const String apip=config->getString(config->apIp);
    bool cfgIpOk=false;
    if (!apip.isEmpty()){
        cfgIpOk= AP_local_ip.fromString(apip);
    }
    if (! cfgIpOk){
        AP_local_ip=IPAddress(192,168,15,1);
        LOG_DEBUG(GwLog::ERROR,"unable to set access point IP %s, falling back to %s",
            apip.c_str(),AP_local_ip.toString().c_str());
    }
    IPAddress AP_gateway(AP_local_ip);
    bool maskOk=false;
    IPAddress AP_subnet;
    const String apMask=config->getString(config->apMask);
    if (!apMask.isEmpty()){
        maskOk=AP_subnet.fromString(apMask);
    }
    if (! maskOk){
        AP_subnet=IPAddress(255, 255, 255, 0);
        LOG_DEBUG(GwLog::ERROR,"unable to set access point mask %s, falling back to %s",
            apMask.c_str(),AP_subnet.toString().c_str());
    }
    //try to remove any existing config from nvs
    //this will avoid issues when updating from framework 6.3.2 to 6.8.1 - see #78
    //we do not need the nvs config any way - so we set persistent to false
    //unfortunately this will be to late (config from nvs has already been loaded)
    //if we update from an older version that has config in the nvs
    //so we need to make a dummy init, erase the flash and deinit
    wifi_config_t conf_current;
    wifi_init_config_t conf=WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err=esp_wifi_init(&conf);
    esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf_current);
    LOG_DEBUG(GwLog::DEBUG,"Wifi AP old config before reset ssid=%s, pass=%s, channel=%d",conf_current.ap.ssid,conf_current.ap.password,conf_current.ap.channel);
    if (err){
        LOG_DEBUG(GwLog::ERROR,"unable to pre-init wifi: %d",(int)err);
    }
    err=esp_wifi_restore();
    if (err){
        LOG_DEBUG(GwLog::ERROR,"unable to reset wifi: %d",(int)err);
    }
    err=esp_wifi_deinit();
    WiFi.persistent(false);
    WiFi.mode(WIFI_MODE_APSTA); //enable both AP and client
    esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf_current);
    LOG_DEBUG(GwLog::DEBUG,"Wifi AP old config after reset ssid=%s, pass=%s, channel=%d",conf_current.ap.ssid,conf_current.ap.password,conf_current.ap.channel);
    const char *ssid=config->getConfigItem(config->systemName)->asCString();
    if (fixedApPass){
        WiFi.softAP(ssid,AP_password);
    }
    else{
        WiFi.softAP(ssid,config->getConfigItem(config->apPassword)->asCString());
    }
    delay(100);
    WiFi.softAPConfig(AP_local_ip, AP_gateway, AP_subnet);
    LOG_DEBUG(GwLog::ERROR,"WifiAP created: ssid=%s,adress=%s",
        ssid,
        WiFi.softAPIP().toString().c_str()
        );
    apActive=true;   
    lastApAccess=millis();
    apShutdownTime=config->getConfigItem(config->stopApTime)->asInt() * 60;
    if (apShutdownTime < 120 && apShutdownTime != 0) apShutdownTime=120; //min 2 minutes
    LOG_DEBUG(GwLog::ERROR,"GWWIFI: AP auto shutdown %s (%ds)",apShutdownTime> 0?"enabled":"disabled",apShutdownTime);
    apShutdownTime=apShutdownTime*1000; //ms   
    clientIsConnected=false;
    connectInternal();    
}
bool GwWifi::connectInternal(){
    if (wifiClient->asBoolean()){
        clientIsConnected=false;
        LOG_DEBUG(GwLog::LOG,"creating wifiClient ssid=%s",wifiSSID->asString().c_str());
        WiFi.setAutoReconnect(false); //#102
        wl_status_t rt=WiFi.begin(wifiSSID->asCString(),wifiPass->asCString());
        LOG_DEBUG(GwLog::LOG,"wifiClient connect returns %d",(int)rt);
        lastConnectStart=millis();
        return true;
    }
    return false;
}
//#102: we should have a wifi connect retry being > 30s - with some headroom
#define RETRY_MILLIS 40000
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
                LOG_DEBUG(GwLog::LOG,"wifiClient now connected to %s at %s",wifiSSID->asCString(),WiFi.localIP().toString().c_str());
                clientIsConnected=true;
            }
        }
    }
    if (apShutdownTime != 0 && apActive){
        if (WiFi.softAPgetStationNum()){
            lastApAccess=millis();
        }
        if ((lastApAccess + apShutdownTime) < millis()){
            LOG_DEBUG(GwLog::ERROR,"GWWIFI: shutdown AP");
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