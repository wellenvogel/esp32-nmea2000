#include "GwLog.h"

class DefaultLogWriter: public GwLogWriter{
    public:
        virtual ~DefaultLogWriter(){};
        virtual void write(const char *data){
            Serial.println(data);
        }
};
void GwLog::startAsync(AsyncLogFunction async){
    if (this->async) return;
    queue = xQueueCreate(50,sizeof(const char *));
    this->async=async;
    if (async){
        xTaskCreate([](void *p){
            GwLog *l=(GwLog*)p;
            while (true){
                char *data=NULL;
                if (xQueueReceive(l->queue,&data,portMAX_DELAY)){
                    (*(l->async))(data);
                    delete data;
                }
            }
        },
        "asyncLog",2000,this,3,NULL);
    }
}
GwLog::GwLog(int level, GwLogWriter *writer){
    logLevel=level;
    if (writer == NULL) writer=new DefaultLogWriter();
    this->writer=writer;
    locker = xSemaphoreCreateMutex();
}
GwLog::~GwLog(){
    vSemaphoreDelete(locker);
}
void GwLog::sendAsync(const char *buf){
    if (async){
        char *abuffer=new char[bufferSize];
        strncpy(abuffer,buf,bufferSize-1);
        abuffer[bufferSize-1]=0;
        if (!xQueueSend(queue,&abuffer,0)){
            delete abuffer;
        }
    }
}
void GwLog::logString(const char *fmt,...){
    va_list args;
    va_start(args,fmt);
    xSemaphoreTake(locker, portMAX_DELAY);
    snprintf(buffer,bufferSize-1,"%s%lu:",prefix.c_str(),millis());
    size_t len=strlen(buffer);
    vsnprintf(buffer+len,bufferSize-1-len,fmt,args);
    buffer[bufferSize-1]=0;
    sendAsync(buffer);
    if (! writer) {
        xSemaphoreGive(locker);
        return;
    }
    writer->write(buffer);
    xSemaphoreGive(locker);
}
void GwLog::logDebug(int level,const char *fmt,...){
    if (level > logLevel) return;
    va_list args;
    va_start(args,fmt);
    xSemaphoreTake(locker, portMAX_DELAY);
    snprintf(buffer,bufferSize-1,"%s%lu:",prefix.c_str(),millis());
    size_t len=strlen(buffer);
    vsnprintf(buffer+len,bufferSize-1-len,fmt,args);
    buffer[bufferSize-1]=0;
    sendAsync(buffer);
    if (! writer) {
        xSemaphoreGive(locker);
        return;
    }
    writer->write(buffer);
    xSemaphoreGive(locker);
}
void GwLog::setWriter(GwLogWriter *writer){
    xSemaphoreTake(locker, portMAX_DELAY);
    if (this->writer) delete this->writer;
    this->writer=writer;
    xSemaphoreGive(locker);
}

void GwLog::flush(){
    xSemaphoreTake(locker, portMAX_DELAY);
    if (! this->writer) {
        xSemaphoreGive(locker);
        return;
    }
    this->writer->flush();
    xSemaphoreGive(locker);
}
