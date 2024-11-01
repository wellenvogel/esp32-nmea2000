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
        bool availableWrite=false; //if this is false we will wait for availabkleWrite until we flush again
    public:
        class SerialWrapperBase{
        public:
        virtual void begin(GwLog* logger,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1)=0;
        virtual int getId()=0;
        virtual int available(){return getStream()->available();}
        size_t readBytes(uint8_t *buffer, size_t length){
            return getStream()->readBytes(buffer,length);
        }
        virtual int availableForWrite(void){
            return getStream()->availableForWrite();
        }
        size_t write(const uint8_t *buffer, size_t size){
            return getStream()->write(buffer,size);
        }
        private:
        virtual Stream *getStream()=0;
        };
        static const int bufferSize=200;
        GwSerial(GwLog *logger,SerialWrapperBase *stream,bool allowRead=true);
        ~GwSerial();
        bool isInitialized();
        virtual size_t sendToClients(const char *buf,int sourceId,bool partial=false);
        virtual void loop(bool handleRead=true,bool handleWrite=true);
        virtual void readMessages(GwMessageFetcher *writer);
        bool flush(long millis=200);
        virtual Stream *getStream(bool partialWrites);
        bool getAvailableWrite(){return availableWrite;}
    friend GwSerialStream;
    private:
        SerialWrapperBase *serial;
};
#endif