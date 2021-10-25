#ifndef _GWSERIAL_H
#define _GWSERIAL_H
#include "driver/uart.h"
#include "GwLog.h"
#include "GwBuffer.h"
class SerialWriter;
class GwSerial{
    private:
        GwBuffer *buffer;
        GwLog *logger; 
        SerialWriter *writer;
        uart_port_t num;
        bool initialized=false;
    public:
        static const int bufferSize=200;
        GwSerial(GwLog *logger,uart_port_t num);
        ~GwSerial();
        int setup(int baud,int rxpin,int txpin);
        bool isInitialized();
        size_t enqueue(const uint8_t *data, size_t len);
        GwBuffer::WriteStatus write();
        const char *read();
};
#endif