#include "GwSerial.h"

class GwSerialStream: public Stream{
    private:
        GwSerial *serial;
        bool partialWrites;
    public:
    GwSerialStream(GwSerial *serial,bool partialWrites=false){
        this->serial=serial;
        this->partialWrites=partialWrites;
    }
    virtual int available(){
        if (! serial->isInitialized()) return 0;
        if (! serial->readBuffer) return 0;
        return serial->readBuffer->usedSpace();
    }
    virtual int read(){
        if (! serial->isInitialized()) return -1;
        if (! serial->readBuffer) return -1;
        return serial->readBuffer->read();
    }
    virtual int peek(){
        if (! serial->isInitialized()) return -1;
        if (! serial->readBuffer) return -1;
        return serial->readBuffer->peek();
    }
    virtual void flush() {};
    virtual size_t write(uint8_t v){
        if (! serial->isInitialized()) return 0;
        size_t rt=serial->buffer->addData(&v,1,partialWrites);
        return rt;
    }
    virtual size_t write(const uint8_t *buffer, size_t size){
        if (! serial->isInitialized()) return 0;
        size_t rt=serial->buffer->addData(buffer,size,partialWrites);
        return rt;
    }

};


class SerialWriter : public GwBufferWriter{
    private:
        HardwareSerial *serial;
    public:
        SerialWriter(HardwareSerial *serial){
            this->serial=serial;
        }
        virtual ~SerialWriter(){}
        virtual int write(const uint8_t *buffer,size_t len){
            size_t numWrite=serial->availableForWrite();
            if (numWrite < len) len=numWrite;
            if (!len) return 0;
            return serial->write(buffer,len);
        }


};
GwSerial::GwSerial(GwLog *logger, int num, int id,bool allowRead)
{
    LOG_DEBUG(GwLog::DEBUG,"creating GwSerial %p port %d",this,(int)num);
    this->id=id;
    this->logger = logger;
    this->num = num;
    this->buffer = new GwBuffer(logger,GwBuffer::TX_BUFFER_SIZE);
    this->allowRead=allowRead;
    if (allowRead){
        this->readBuffer=new GwBuffer(logger, GwBuffer::RX_BUFFER_SIZE);
    }
    this->serial=new HardwareSerial(num);
    this->writer = new SerialWriter(serial);

}
GwSerial::~GwSerial()
{
    delete buffer;
    delete writer;
    if (readBuffer) delete readBuffer;
    delete serial;
}
int GwSerial::setup(int baud, int rxpin, int txpin)
{
    serial->begin(baud,SERIAL_8N1,rxpin,txpin);
    buffer->reset();
    initialized = true;
    return 0;
}
bool GwSerial::isInitialized() { return initialized; }
size_t GwSerial::enqueue(const uint8_t *data, size_t len, bool partial)
{
    if (! isInitialized()) return 0;
    return buffer->addData(data, len,partial);
}
GwBuffer::WriteStatus GwSerial::write(){
    if (! isInitialized()) return GwBuffer::ERROR;
    return buffer->fetchData(writer,-1,false);
}
size_t GwSerial::sendToClients(const char *buf,int sourceId,bool partial){
    if ( sourceId == id) return 0;
    size_t len=strlen(buf);
    size_t enqueued=enqueue((const uint8_t*)buf,len,partial);
    if (enqueued != len && ! partial){
        LOG_DEBUG(GwLog::DEBUG,"GwSerial overflow on channel %d",id);
        overflows++;
    }
    return enqueued;
}
void GwSerial::loop(bool handleRead){
    write();
    if (! isInitialized()) return;
    if (! handleRead) return;
    char buffer[10];
    size_t available=serial->available();
    if (! available) return;
    if (available > 10) available=10;
    int rt=serial->readBytes(buffer,available);
    if (allowRead && rt > 0){
        size_t wr=readBuffer->addData((uint8_t *)(&buffer),rt,true);
        LOG_DEBUG(GwLog::DEBUG+2,"GwSerial read %d bytes, to buffer %d bytes",rt,wr);
    }
}
bool GwSerial::readMessages(GwBufferWriter *writer){
    if (! isInitialized()) return false;
    if (! allowRead) return false;
    return readBuffer->fetchMessage(writer,'\n',true) == GwBuffer::OK;
}

void GwSerial::flush(){
   if (! isInitialized()) return; 
   while (buffer->fetchData(writer,-1,false) == GwBuffer::AGAIN){
       vTaskDelay(1);
   }
}
Stream * GwSerial::getStream(bool partialWrite){
    return new GwSerialStream(this,partialWrite);
}
