#ifndef _GWLOG_H
#define _GWLOG_H
#include <Arduino.h>
class GwLog{
    private:
        char buffer[100];
        bool logSerial=false;
    public:
        GwLog(bool logSerial);
        void logString(const char *fmt,...);
};
#endif