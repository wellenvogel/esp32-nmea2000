#ifndef _GWSERIAL_H
#define _GWSERIAL_H
#include "HardwareSerial.h"
#include "GwLog.h"
#include "GwBuffer.h"
class SerialWriter;
class GwSerialStream;
class GwSerial{
    private:
        GwBuffer *buffer;
        GwBuffer *readBuffer=NULL;
        GwLog *logger; 
        SerialWriter *writer;
        int num;
        bool initialized=false;
        bool allowRead=true;
        GwBuffer::WriteStatus write();
        int id=-1;
        int overflows=0;
        size_t enqueue(const uint8_t *data, size_t len,bool partial=false);
        HardwareSerial *serial;
    public:
        static const int bufferSize=200;
        GwSerial(GwLog *logger,int num,int id,bool allowRead=true);
        ~GwSerial();
        int setup(int baud,int rxpin,int txpin);
        bool isInitialized();
        size_t sendToClients(const char *buf,int sourceId,bool partial=false);
        void loop(bool handleRead=true);
        bool readMessages(GwBufferWriter *writer);
        void flush();
        Stream *getStream(bool partialWrites);
    friend GwSerialStream;
};
#endif