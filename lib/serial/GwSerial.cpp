#include "GwSerial.h"
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
size_t GwSerial::enqueue(const uint8_t *data, size_t len)
{
    if (! isInitialized()) return 0;
    return buffer->addData(data, len);
}
GwBuffer::WriteStatus GwSerial::write(){
    if (! isInitialized()) return GwBuffer::ERROR;
    return buffer->fetchData(writer,-1,false);
}
void GwSerial::sendToClients(const char *buf,int sourceId){
    if ( sourceId == id) return;
    size_t len=strlen(buf);
    size_t enqueued=enqueue((const uint8_t*)buf,len);
    if (enqueued != len){
        LOG_DEBUG(GwLog::DEBUG,"GwSerial overflow on channel %d",id);
        overflows++;
    }
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