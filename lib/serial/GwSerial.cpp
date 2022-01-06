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



GwSerial::GwSerial(GwLog *logger, int num, int id,bool allowRead)
{
    LOG_DEBUG(GwLog::DEBUG,"creating GwSerial %p port %d for %d",this,(int)num,id);
    this->id=id;
    this->logger = logger;
    this->num = num;
    String bufName="Ser(";
    bufName+=String(id);
    bufName+=")";
    this->buffer = new GwBuffer(logger,GwBuffer::TX_BUFFER_SIZE,bufName+"wr");
    this->allowRead=allowRead;
    if (allowRead){
        this->readBuffer=new GwBuffer(logger, GwBuffer::RX_BUFFER_SIZE,bufName+"rd");
    }
    this->serial=new HardwareSerial(num);
}
GwSerial::~GwSerial()
{
    delete buffer;
    if (readBuffer) delete readBuffer;
    delete serial;
}
int GwSerial::setup(int baud, int rxpin, int txpin)
{
    serial->begin(baud,SERIAL_8N1,rxpin,txpin);
    buffer->reset(F("init"));
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
    size_t numWrite=serial->availableForWrite();          
    size_t rt=buffer->fetchData(numWrite,[](uint8_t *buffer,size_t len, void *p){
        return ((GwSerial *)p)->serial->write(buffer,len);
    },this);
    if (rt != 0){
        LOG_DEBUG(GwLog::DEBUG+1,"Serial %d write %d",id,rt);
    }
    return buffer->usedSpace()?GwBuffer::AGAIN:GwBuffer::OK;
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
void GwSerial::loop(bool handleRead,bool handleWrite){
    write();
    if (! isInitialized()) return;
    if (! handleRead) return;
    size_t available=serial->available();
    if (! available) return;
    if (allowRead){
        size_t rd=readBuffer->fillData(available,[](uint8_t *buffer, size_t len, void *p)->size_t{
            return ((GwSerial *)p)->serial->readBytes(buffer,len);
        },this);
        if (rd != 0){
            LOG_DEBUG(GwLog::DEBUG+2,"GwSerial %d read %d bytes",id,rd);
        }
    }
    else{
        uint8_t buffer[10];
        if (available > 10) available=10;
        serial->readBytes(buffer,available);
    }
}
void GwSerial::readMessages(GwMessageFetcher *writer){
    if (! isInitialized()) return;
    if (! allowRead) return;
    writer->handleBuffer(readBuffer);
}

void GwSerial::flush(){
   if (! isInitialized()) return; 
   while (write() == GwBuffer::AGAIN){
       vTaskDelay(1);
   }
}
Stream * GwSerial::getStream(bool partialWrite){
    return new GwSerialStream(this,partialWrite);
}
