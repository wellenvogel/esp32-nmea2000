#ifndef _GWLOG_H
#define _GWLOG_H
#include <Arduino.h>
class GwLog{
    private:
        char buffer[100];
        bool logSerial=false;
        int logLevel=1;
    public:
        static const int LOG=1;
        static const int ERROR=0;
        static const int DEBUG=3;
        static const int TRACE=2;
        GwLog(bool logSerial,int level=LOG);
        void logString(const char *fmt,...);
        void logDebug(int level, const char *fmt,...);
        int isActive(int level){return level <= logLevel;};
};
#define LOG_DEBUG(level,fmt,...){ if (logger->isActive(level)) logger->logDebug(level,fmt,__VA_ARGS__);}

#endif