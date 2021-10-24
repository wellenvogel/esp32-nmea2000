#include "GwLog.h"

GwLog::GwLog(bool logSerial, int level){
    this->logSerial=logSerial;
    logLevel=level;
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
void GwLog::logDebug(int level,const char *fmt,...){
    if (level > logLevel) return;
    va_list args;
    va_start(args,fmt);
    vsnprintf(buffer,99,fmt,args);
    if (logSerial){
        Serial.print("LOG: ");
        Serial.println(buffer);
    }
}
