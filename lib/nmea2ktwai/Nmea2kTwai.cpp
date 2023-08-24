#include "Nmea2kTwai.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "GwLog.h"

#define TWAI_DEBUG 0
#if TWAI_DEBUG == 1
    #define TWAI_LOG(...) LOG_DEBUG(__VA_ARGS__)
    #define TWAI_LDEBUG(...)
#else
  #if TWAI_DEBUG == 2
    #define TWAI_LOG(...) LOG_DEBUG(__VA_ARGS__)
    #define TWA_LDEBUG(...) LOG_DEBUG(__VA_ARGS__)
  #else
    #define TWAI_LOG(...)
    #define TWAI_LDEBUG(...)
  #endif  
#endif

Nmea2kTwai::Nmea2kTwai(gpio_num_t _TxPin,  gpio_num_t _RxPin,GwLog *l):
     tNMEA2000(),RxPin(_RxPin),TxPin(_TxPin),logger(l)
{
}

bool Nmea2kTwai::CANSendFrame(unsigned long id, unsigned char len, const unsigned char *buf, bool wait_sent)
{
    if (recoveryStarted){
        if (getState() == ST_RUNNING){
            recoveryStarted=false;
        }
        else{
            return false;
        }
    }
    twai_message_t message;
    memset(&message,0,sizeof(message));
    message.identifier = id;
    message.extd = 1;
    message.data_length_code = len;
    memcpy(message.data,buf,len);
    esp_err_t rt=twai_transmit(&message,0);
    if (rt != ESP_OK){
        TWAI_LOG(GwLog::DEBUG,"twai transmit for %ld failed: %x",(id & 0x1ffff),(int)rt);
        return false;
    }
    TWAI_LDEBUG(GwLog::DEBUG,"twai transmit id %ld, len %d",(id & 0x1ffff),(int)len);
    return true;
}
bool Nmea2kTwai::CANOpen()
{
    esp_err_t rt=twai_start();
    if (rt != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"CANOpen failed: %x",(int)rt);
        return false;
    }
    else{
        LOG_DEBUG(GwLog::LOG,"CANOpen ok");
    }
    return true;
}
bool Nmea2kTwai::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf)
{
    if (recoveryStarted){
        if (getState() == ST_RUNNING){
            recoveryStarted=false;
        }
        else{
            return false;
        }
    }
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
        TWAI_LOG(GwLog::ERROR,"twai: received invalid message %lld, len %d",id,len);
        len=8;
    }
    TWAI_LDEBUG(GwLog::DEBUG,"twai rcv id=%ld,len=%d, ext=%d",message.identifier,message.data_length_code,message.extd);
    if (! message.rtr){
        memcpy(buf,message.data,message.data_length_code);
    }
    return true;
}
// This will be called on Open() before any other initialization. Inherit this, if buffers can be set for the driver
// and you want to change size of library send frame buffer size. See e.g. NMEA2000_teensy.cpp.
void Nmea2kTwai::InitCANFrameBuffers()
{
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TxPin,RxPin, TWAI_MODE_NORMAL);
    g_config.tx_queue_len=20;
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    esp_err_t rt=twai_driver_install(&g_config, &t_config, &f_config);
    if (rt == ESP_OK) {
        LOG_DEBUG(GwLog::LOG,"twai driver initialzed, rx=%d,tx=%d",(int)RxPin,(int)TxPin);
    }
    else{
        LOG_DEBUG(GwLog::ERROR,"twai driver init failed: %x",(int)rt);
    }
    tNMEA2000::InitCANFrameBuffers();
    
}
Nmea2kTwai::STATE Nmea2kTwai::getState(){
    twai_status_info_t state;
    if (twai_get_status_info(&state) != ESP_OK){
        return ST_ERROR;
    }
    switch(state.state){
        case TWAI_STATE_STOPPED:
            return ST_STOPPED;
        case TWAI_STATE_RUNNING:
            return ST_RUNNING;
        case TWAI_STATE_BUS_OFF:
            return ST_BUS_OFF;
        case TWAI_STATE_RECOVERING:
            return ST_RECOVERING;
    }
    return ST_ERROR;
}
Nmea2kTwai::ERRORS Nmea2kTwai::getErrors(){
    ERRORS rt;
    twai_status_info_t state;
    if (twai_get_status_info(&state) != ESP_OK){
        return rt;
    }
    rt.rx_errors=state.rx_error_counter;
    rt.tx_errors=state.tx_error_counter;
    rt.tx_failed=state.tx_failed_count;
    return rt;
}
bool Nmea2kTwai::startRecovery(){
    if (! recoveryStarted){
        recoveryStarted=true; //prevent any further sends
        return true;
    }
    esp_err_t rt=twai_initiate_recovery();
    if (rt != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"twai: initiate recovery failed with error %x",(int)rt);
        return false;
    }
    recoveryStarted=true;
    LOG_DEBUG(GwLog::LOG,"twai: bus recovery started");
    return true;
}
const char * Nmea2kTwai::stateStr(const Nmea2kTwai::STATE &st){
    switch (st)
    {
    case ST_BUS_OFF: return "BUS_OFF";
    case ST_RECOVERING: return "RECOVERING";
    case ST_RUNNING: return "RUNNING";
    case ST_STOPPED: return "STOPPED";
    }
    return "ERROR";
}