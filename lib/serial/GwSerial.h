#ifndef _GWSERIAL_H
#define _GWSERIAL_H
#include "HardwareSerial.h"
#include "GwLog.h"
#include "GwBuffer.h"
#include "GwChannelInterface.h"
class GwSerialStream;
class GwSerial : public GwChannelInterface{
    private:
        GwBuffer *buffer;
        GwBuffer *readBuffer=NULL;
        GwLog *logger; 
        bool initialized=false;
        bool allowRead=true;
        GwBuffer::WriteStatus write();
        int id=-1;
        int overflows=0;
        size_t enqueue(const uint8_t *data, size_t len,bool partial=false);
        Stream *serial;
    public:
        static const int bufferSize=200;
        GwSerial(GwLog *logger,Stream *stream,int id,bool allowRead=true);
        ~GwSerial();
        bool isInitialized();
        virtual size_t sendToClients(const char *buf,int sourceId,bool partial=false);
        virtual void loop(bool handleRead=true,bool handleWrite=true);
        virtual void readMessages(GwMessageFetcher *writer);
        void flush();
        virtual Stream *getStream(bool partialWrites);
    friend GwSerialStream;
};
#endif