#include "GwLog.h"

GwLog::GwLog(bool logSerial){
    this->logSerial=logSerial;
}
void GwLog::logString(const char *fmt,...){
    va_list args;
    va_start(args,fmt);
    vsnprintf(buffer,99,fmt,args);
    if (logSerial){
        Serial.print("LOG: ");
        Serial.println(buffer);
    }
}