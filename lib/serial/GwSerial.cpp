#include "GwSerial.h"
class SerialWriter : public GwBufferWriter{
    private:
        uart_port_t num;
    public:
        SerialWriter(uart_port_t num){
            this->num=num;
        }
        virtual ~SerialWriter(){}
        virtual int write(const uint8_t *buffer,size_t len){
            return uart_tx_chars(num,(const char *)buffer,len);
        }


};
GwSerial::GwSerial(GwLog *logger, uart_port_t num, int id,bool allowRead)
{
    LOG_DEBUG(GwLog::DEBUG,"creating GwSerial %p port %d",this,(int)num);
    this->id=id;
    this->logger = logger;
    this->num = num;
    this->buffer = new GwBuffer(logger,GwBuffer::TX_BUFFER_SIZE);
    this->writer = new SerialWriter(num);
    this->allowRead=allowRead;
    if (allowRead){
        this->readBuffer=new GwBuffer(logger, GwBuffer::RX_BUFFER_SIZE);
    }

}
GwSerial::~GwSerial()
{
    delete buffer;
    delete writer;
    if (readBuffer) delete readBuffer;
}
int GwSerial::setup(int baud, int rxpin, int txpin)
{
    uart_config_t config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    if (uart_driver_install(num, bufferSize, 0, 0, NULL, 0) != ESP_OK)
    {
        return -1;
    }
    // Configure UART parameters
    if (uart_param_config(num, &config) != ESP_OK)
    {
        return -2;
    }
    if (uart_set_pin(num, txpin, rxpin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
    {
        return -3;
    }
    buffer->reset();
    initialized = true;
    return true;
}
bool GwSerial::isInitialized() { return initialized; }
size_t GwSerial::enqueue(const uint8_t *data, size_t len)
{
    if (! isInitialized()) return 0;
    return buffer->addData(data, len);
}
GwBuffer::WriteStatus GwSerial::write(){
    if (! isInitialized()) return GwBuffer::ERROR;
    return buffer->fetchData(writer,false);
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
    int rt=uart_read_bytes(num,(uint8_t *)(&buffer),10,0);
    if (allowRead && rt > 0){
        readBuffer->addData((uint8_t *)(&buffer),rt,true);
    }
}
bool GwSerial::readMessages(GwBufferWriter *writer){
    if (! isInitialized()) return false;
    if (! allowRead) return false;
    return readBuffer->fetchMessage(writer,'\n',true) == GwBuffer::OK;
}