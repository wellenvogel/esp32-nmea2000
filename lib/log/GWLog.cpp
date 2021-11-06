#include "GwLog.h"

class DefaultLogWriter: public GwLogWriter{
    public:
        virtual ~DefaultLogWriter(){};
        virtual void write(const char *data){
            Serial.print(data);
        }
};

GwLog::GwLog(int level, GwLogWriter *writer){
    logLevel=level;
    if (writer == NULL) writer=new DefaultLogWriter();
    this->writer=writer;
    locker = xSemaphoreCreateMutex();
}
GwLog::~GwLog(){
    vSemaphoreDelete(locker);
}
void GwLog::logString(const char *fmt,...){
    va_list args;
    va_start(args,fmt);
    xSemaphoreTake(locker, portMAX_DELAY);
    vsnprintf(buffer,99,fmt,args);
    if (! writer) return;
    writer->write(prefix.c_str());
    writer->write(buffer);
    writer->write("\n");
    xSemaphoreGive(locker);
}
void GwLog::logDebug(int level,const char *fmt,...){
    if (level > logLevel) return;
    va_list args;
    va_start(args,fmt);
    xSemaphoreTake(locker, portMAX_DELAY);
    vsnprintf(buffer,99,fmt,args);
    if (! writer) return;
    writer->write(prefix.c_str());
    writer->write(buffer);
    writer->write("\n");
    xSemaphoreGive(locker);
}
void GwLog::setWriter(GwLogWriter *writer){
    xSemaphoreTake(locker, portMAX_DELAY);
    if (this->writer) delete this->writer;
    this->writer=writer;
    xSemaphoreGive(locker);
}
