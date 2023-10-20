#include "GwLog.h"
#include "GwHardware.h"


GwLog::GwLog(int level, GwLogWriter *writer){
    logLevel=level;
    this->writer=writer;
    if (!writer){
        iniBuffer=new char[INIBUFFERSIZE];
        iniBuffer[0]=0;
    }
    locker = xSemaphoreCreateMutex();
}
GwLog::~GwLog(){
    vSemaphoreDelete(locker);
}
void GwLog::writeOut(const char *data)
{
    if (!writer)
    {
        if (iniBuffer && iniBufferFill < (INIBUFFERSIZE - 1))
        {
            size_t remain = INIBUFFERSIZE - iniBufferFill-1;
            size_t len = strlen(data);
            if (len < remain)
                remain = len;
            if (remain){
                memcpy(iniBuffer + iniBufferFill, data, remain);
                iniBufferFill += remain;
                iniBuffer[iniBufferFill] = 0;
            }
        }
    }
    else
    {
        writer->write(data);
    }
}
void GwLog::logString(const char *fmt,...){
    va_list args;
    va_start(args,fmt);
    xSemaphoreTake(locker, portMAX_DELAY);
    recordCounter++;
    vsnprintf(buffer,bufferSize-1,fmt,args);
    buffer[bufferSize-1]=0;
    writeOut(prefix.c_str());
    char buf[20];
    snprintf(buf,20,"%lu:",millis());
    writeOut(buf);
    writeOut(buffer);
    writeOut("\n");
    xSemaphoreGive(locker);
}
void GwLog::logDebug(int level,const char *fmt,...){
    va_list args;
    va_start(args,fmt);
    logDebug(level,fmt,args);
}
void GwLog::logDebug(int level,const char *fmt,va_list args){
    if (level > logLevel) return;
    xSemaphoreTake(locker, portMAX_DELAY);
    recordCounter++;
    vsnprintf(buffer,bufferSize-1,fmt,args);
    buffer[bufferSize-1]=0;
    writeOut(prefix.c_str());
    char buf[20];
    snprintf(buf,20,"%lu:",millis());
    writeOut(buf);
    writeOut(buffer);
    writeOut("\n");
    xSemaphoreGive(locker);
}
void GwLog::setWriter(GwLogWriter *writer){
    xSemaphoreTake(locker, portMAX_DELAY);
    if (this->writer) delete this->writer;
    this->writer=writer;
    if (iniBuffer && iniBufferFill){
        writer->write(iniBuffer);
        iniBufferFill=0;
        delete[] iniBuffer;
        iniBuffer=nullptr;
    }
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
