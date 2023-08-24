#ifndef _NMEA2KTWAI_H
#define _NMEA2KTWAI_H
#include "NMEA2000.h"
#include "GwLog.h"

class Nmea2kTwai : public tNMEA2000{
    public:
        Nmea2kTwai(gpio_num_t _TxPin,  gpio_num_t _RxPin,GwLog *logger);
        typedef enum{
            ST_STOPPED,
            ST_RUNNING,
            ST_BUS_OFF,
            ST_RECOVERING,
            ST_ERROR
        } STATE;
        STATE getState();
        bool startRecovery();
    protected:
    // Virtual functions for different interfaces. Currently there are own classes
    // for Arduino due internal CAN (NMEA2000_due), external MCP2515 SPI CAN bus controller (NMEA2000_mcp),
    // Teensy FlexCAN (NMEA2000_Teensy), NMEA2000_avr for AVR, NMEA2000_mbed for MBED and NMEA2000_socketCAN for e.g. RPi.
    virtual bool CANSendFrame(unsigned long id, unsigned char len, const unsigned char *buf, bool wait_sent=true);
    virtual bool CANOpen();
    virtual bool CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf);
    // This will be called on Open() before any other initialization. Inherit this, if buffers can be set for the driver
    // and you want to change size of library send frame buffer size. See e.g. NMEA2000_teensy.cpp.
    virtual void InitCANFrameBuffers();

    

    private:
    gpio_num_t TxPin;  
    gpio_num_t RxPin;
    GwLog *logger;

};

#endif