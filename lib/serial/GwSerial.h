#ifndef _GWSERIAL_H
#define _GWSERIAL_H
#include "HardwareSerial.h"
#include "GwLog.h"
#include "GwBuffer.h"
#include "GwChannelInterface.h"
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
  #include "hal/usb_serial_jtag_ll.h"
#endif

#define USBCDC_RESTART_TIME 100
class GwSerialStream;
class GwSerial : public GwChannelInterface{
    protected:
        GwBuffer *buffer;
        GwBuffer *readBuffer=NULL;
        GwLog *logger; 
        Stream *stream;
        bool initialized=false;
        bool allowRead=true;
        GwBuffer::WriteStatus write();
        int id=-1;
        int overflows=0;
        size_t enqueue(const uint8_t *data, size_t len,bool partial=false);
        bool availableWrite=false; //if this is false we will wait for availabkleWrite until we flush again
        virtual long getFlushTimeout(){return 2000;}
        virtual int availableForWrite()=0;
        int type=0;
    public:
        GwSerial(GwLog *logger,Stream *stream,int id,int type,bool allowRead=true);
        virtual ~GwSerial();
        bool isInitialized();
        virtual size_t sendToClients(const char *buf,int sourceId,bool partial=false);
        virtual void loop(bool handleRead=true,bool handleWrite=true);
        virtual void readMessages(GwMessageFetcher *writer);
        bool flush();
        virtual Stream *getStream(bool partialWrites);
        bool getAvailableWrite(){return availableWrite;}
        virtual void begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1)=0;
        virtual String getMode() override;
    friend GwSerialStream;
};

template<typename T>
    class GwSerialImpl : public GwSerial{
        private:
        unsigned long lastWritable=0;
        template<class C>
        void beginImpl(C *s,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1){}
        void beginImpl(HardwareSerial *s,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1){
            s->begin(baud,config,rxPin,txPin);
        }
        template<class C>
        void setError(C* s, GwLog *logger){}
        void setError(HardwareSerial *s,GwLog *logger){
            LOG_DEBUG(GwLog::LOG,"enable serial errors for channel %d",id);
            s->onReceiveError([logger,this](hardwareSerial_error_t err){
                LOG_DEBUG(GwLog::ERROR,"serial error on id %d: %d",this->id,(int)err);
            });
        }
        #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
            void beginImpl(HWCDC *s,unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1){
            s->begin(baud);
        }
        #endif
        template<class C>
        long getFlushTimeoutImpl(const C*){return 2000;}
        #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
            long getFlushTimeoutImpl(HWCDC *){return 200;}
        #endif

        template<class C>
        int availableForWrite(C* c){
            return c->availableForWrite();
        }

        #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
        /**
         * issue #81
         * workaround for the HWCDC beeing stuck at some point in time
         * with availableForWrite == 0 but the ISR being disabled
         * we simply give a small delay of 100ms for availableForWrite being 0
         * and afterwards retrigger the ISR
        */
        int availableForWrite(HWCDC* c){
            int rt=c->availableForWrite();
            if (rt > 0) {
                lastWritable=millis();
                return rt;
            }
            unsigned long now=millis();
            if (now > (lastWritable+USBCDC_RESTART_TIME)){
                lastWritable=now;
                if (c->isConnected()){
                    //this retriggers the ISR
                    usb_serial_jtag_ll_ena_intr_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
                }
            }
            return rt;
        }
        #endif

        T *serial;
        protected:
        virtual long getFlushTimeout() override{
            return getFlushTimeoutImpl(serial);
        }
        virtual int availableForWrite(){
            return availableForWrite(serial);
        }
        public:
        GwSerialImpl(GwLog* logger,T* s,int i,int type,bool allowRead=true): GwSerial(logger,s,i,type,allowRead),serial(s){}
        virtual ~GwSerialImpl(){}
        virtual void begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1) override{
            beginImpl(serial,baud,config,rxPin,txPin);
            setError(serial,logger);
        };

    };


#endif