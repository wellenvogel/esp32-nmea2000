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
    public:
        typedef void (*AsyncLogFunction)(const char *); 
    private:
        static const size_t bufferSize=200;
        char buffer[bufferSize];
        int logLevel=1;
        int asyncLevel=1;
        GwLogWriter *writer;
        unsigned long levelSetTime=0;
        unsigned long levelKeepTime=0;
        SemaphoreHandle_t locker;
        QueueHandle_t queue;
        AsyncLogFunction async=NULL;
        void sendAsync(const char *buf);
        void checkLevel();
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
        int isActive(int level){return level <= logLevel || level <= asyncLevel;};
        void flush();
        void startAsync(AsyncLogFunction function);
        void setAsyncLevel(int level, unsigned long keepTime=10000);
};
#define LOG_DEBUG(level,...){ if (logger != NULL && logger->isActive(level)) logger->logDebug(level,__VA_ARGS__);}

#endif