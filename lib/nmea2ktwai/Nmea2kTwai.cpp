#include "Nmea2kTwai.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "GwLog.h"

#define LOGID(id) ((id >> 8) & 0x1ffff)

static const int TIMEOUT_OFFLINE=256; //# of timeouts to consider offline

Nmea2kTwai::Nmea2kTwai(gpio_num_t _TxPin,  gpio_num_t _RxPin, unsigned long recP):
     tNMEA2000(),RxPin(_RxPin),TxPin(_TxPin),recoveryPeriod(recP)
{
    lastRecoveryCheck=millis();
}

bool Nmea2kTwai::CANSendFrame(unsigned long id, unsigned char len, const unsigned char *buf, bool wait_sent)
{
    twai_message_t message;
    memset(&message,0,sizeof(message));
    message.identifier = id;
    message.extd = 1;
    message.data_length_code = len;
    memcpy(message.data,buf,len);
    esp_err_t rt=twai_transmit(&message,0);
    if (rt != ESP_OK){
        if (rt == ESP_ERR_TIMEOUT){
            if (txTimeouts < TIMEOUT_OFFLINE) txTimeouts++;
        }
        logDebug(LOG_MSG,"twai transmit for %ld failed: %x",LOGID(id),(int)rt);
        return false;
    }
    txTimeouts=0;
    logDebug(LOG_MSG,"twai transmit id %ld, len %d",LOGID(id),(int)len);
    return true;
}
bool Nmea2kTwai::CANOpen()
{
    esp_err_t rt=twai_start();
    if (rt != ESP_OK){
        logDebug(LOG_ERR,"CANOpen failed: %x",(int)rt);
        return false;
    }
    else{
        logDebug(LOG_INFO,"CANOpen ok");
    }
    return true;
}
bool Nmea2kTwai::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf)
{
    twai_message_t message;
    esp_err_t rt=twai_receive(&message,0);
    if (rt != ESP_OK){
        return false;
    }
    if (! message.extd){
        return false;
    }
    id=message.identifier;
    len=message.data_length_code;
    if (len > 8){
        logDebug(LOG_DEBUG,"twai: received invalid message %lld, len %d",LOGID(id),len);
        len=8;
    }
    logDebug(LOG_MSG,"twai rcv id=%ld,len=%d, ext=%d",LOGID(message.identifier),message.data_length_code,message.extd);
    if (! message.rtr){
        memcpy(buf,message.data,message.data_length_code);
    }
    return true;
}
void Nmea2kTwai::initDriver(){
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TxPin,RxPin, TWAI_MODE_NORMAL);
    g_config.tx_queue_len=20;
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    esp_err_t rt=twai_driver_install(&g_config, &t_config, &f_config);
    if (rt == ESP_OK) {
        logDebug(LOG_INFO,"twai driver initialzed, rx=%d,tx=%d",(int)RxPin,(int)TxPin);
    }
    else{
        logDebug(LOG_ERR,"twai driver init failed: %x",(int)rt);
    }
}
// This will be called on Open() before any other initialization. Inherit this, if buffers can be set for the driver
// and you want to change size of library send frame buffer size. See e.g. NMEA2000_teensy.cpp.
void Nmea2kTwai::InitCANFrameBuffers()
{
    initDriver();
    tNMEA2000::InitCANFrameBuffers();
    
}
Nmea2kTwai::Status Nmea2kTwai::getStatus(){
    twai_status_info_t state;
    Status rt;
    if (twai_get_status_info(&state) != ESP_OK){
        return rt;
    }
    switch(state.state){
        case TWAI_STATE_STOPPED:
            rt.state=ST_STOPPED;
            break;
        case TWAI_STATE_RUNNING:
            rt.state=ST_RUNNING;
            break;
        case TWAI_STATE_BUS_OFF:
            rt.state=ST_BUS_OFF;
            break;
        case TWAI_STATE_RECOVERING:
            rt.state=ST_RECOVERING;
            break;
    }
    rt.rx_errors=state.rx_error_counter;
    rt.tx_errors=state.tx_error_counter;
    rt.tx_failed=state.tx_failed_count;
    rt.rx_missed=state.rx_missed_count;
    rt.rx_overrun=state.rx_overrun_count;
    rt.tx_timeouts=txTimeouts;
    if (rt.tx_timeouts >= TIMEOUT_OFFLINE && rt.state == ST_RUNNING){
        rt.state=ST_OFFLINE;
    }
    return rt;
}
bool Nmea2kTwai::checkRecovery(){
    if (recoveryPeriod == 0) return false; //no check
    unsigned long now=millis();
    if ((lastRecoveryCheck+recoveryPeriod) > now) return false;
    lastRecoveryCheck=now;
    Status canState=getStatus();
    bool strt=false;
    if (canState.state != Nmea2kTwai::ST_RUNNING){
      if (canState.state == Nmea2kTwai::ST_BUS_OFF){
        strt=true;
        bool rt=startRecovery();
        logDebug(LOG_INFO,"start can recovery - result %d",(int)rt);
      }
      if (canState.state == Nmea2kTwai::ST_STOPPED){
        bool rt=CANOpen();
        logDebug(LOG_INFO,"restart can driver - result %d",(int)rt);
      }
    }
    return strt;
}

bool Nmea2kTwai::startRecovery(){
    esp_err_t rt=twai_driver_uninstall();
    if (rt != ESP_OK){
        logDebug(LOG_ERR,"twai: deinit for recovery failed with %x",(int)rt);
    }
    initDriver();
    bool frt=CANOpen();
    return frt;
}
const char * Nmea2kTwai::stateStr(const Nmea2kTwai::STATE &st){
    switch (st)
    {
    case ST_BUS_OFF: return "BUS_OFF";
    case ST_RECOVERING: return "RECOVERING";
    case ST_RUNNING: return "RUNNING";
    case ST_STOPPED: return "STOPPED";
    case ST_OFFLINE: return "OFFLINE";
    }
    return "ERROR";
}