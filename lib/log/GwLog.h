#ifndef _GWLOG_H
#define _GWLOG_H
#include <Arduino.h>

class GwLogWriter{
    public:
        virtual ~GwLogWriter(){}
        virtual void write(const char *data)=0;
        virtual void flush(){};
};
class GwLog{
    private:
        static const size_t bufferSize=250;
        char buffer[bufferSize];
        int logLevel=1;
        GwLogWriter *writer;
        SemaphoreHandle_t locker;
    public:
        static const int LOG=1;
        static const int ERROR=0;
        static const int DEBUG=3;
        static const int TRACE=2;
        String prefix="LOG:";
        GwLog(int level=LOG, GwLogWriter *writer=NULL);
        ~GwLog();
        void setWriter(GwLogWriter *writer);
        void logString(const char *fmt,...);
        void logDebug(int level, const char *fmt,...);
        int isActive(int level){return level <= logLevel;};
        void flush();
        void setLevel(int level){this->logLevel=level;}
};
#define LOG_DEBUG(level,...){ if (logger != NULL && logger->isActive(level)) logger->logDebug(level,__VA_ARGS__);}

#endif