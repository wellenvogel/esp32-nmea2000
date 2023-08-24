#include "Nmea2kTwai.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "GwLog.h"

#define TWAI_DEBUG 1
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
    twai_message_t message;
    message.identifier = id;
    message.extd = 1;
    message.data_length_code = len;
    memcpy(message.data,buf,len);
    esp_err_t rt=twai_transmit(&message,0);
    if (rt != ESP_OK){
        TWAI_LOG(GwLog::DEBUG,"twai transmit for %ld failed: %d",id,(int)rt);
        return false;
    }
    TWAI_LDEBUG(GwLog::DEBUG,"twai transmit id %ld, len %d",id,(int)len);
    return true;
}
bool Nmea2kTwai::CANOpen()
{
    esp_err_t rt=twai_start();
    if (rt != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"CANOpen failed: %d",(int)rt);
        return false;
    }
    else{
        LOG_DEBUG(GwLog::LOG,"CANOpen ok");
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
    id=message.identifier;
    len=message.data_length_code;
    TWAI_LDEBUG(GwLog::DEBUG,"twai rcv id=%ld,len=%d, ext=%d",message.identifier,message.data_length_code,message.extd);
    memcpy(buf,message.data,message.data_length_code);
    return true;
}
// This will be called on Open() before any other initialization. Inherit this, if buffers can be set for the driver
// and you want to change size of library send frame buffer size. See e.g. NMEA2000_teensy.cpp.
void Nmea2kTwai::InitCANFrameBuffers()
{
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TxPin,RxPin, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    esp_err_t rt=twai_driver_install(&g_config, &t_config, &f_config);
    if (rt == ESP_OK) {
        LOG_DEBUG(GwLog::LOG,"twai driver initialzed, rx=%d,tx=%d",(int)RxPin,(int)TxPin);
    }
    else{
        LOG_DEBUG(GwLog::ERROR,"twai driver init failed: %d",(int)rt);
    }
    tNMEA2000::InitCANFrameBuffers();
    
}