#ifndef _GWSERIAL_H
#define _GWSERIAL_H
#include "driver/uart.h"
#include "GwLog.h"
#include "GwBuffer.h"
class SerialWriter;
class GwSerial{
    private:
        GwBuffer *buffer;
        GwBuffer *readBuffer=NULL;
        GwLog *logger; 
        SerialWriter *writer;
        uart_port_t num;
        bool initialized=false;
        bool allowRead=true;
        GwBuffer::WriteStatus write();
        int id=-1;
        int overflows=0;
        size_t enqueue(const uint8_t *data, size_t len);
    public:
        static const int bufferSize=200;
        GwSerial(GwLog *logger,uart_port_t num,int id,bool allowRead=true);
        ~GwSerial();
        int setup(int baud,int rxpin,int txpin);
        bool isInitialized();
        void sendToClients(const char *buf,int sourceId);
        void loop(bool handleRead=true);
        bool readMessages(GwBufferWriter *writer);
        void flush();
};
#endif