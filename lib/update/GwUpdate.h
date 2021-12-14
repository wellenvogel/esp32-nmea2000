#pragma once
#include "GwWebServer.h"
class GwUpdate{
    public:
        typedef bool (*PasswordChecker)(String hash);
    private:
        AsyncWebServer *server;
        GwLog *logger;
        PasswordChecker checker;
        bool updateRunning=false;
    public:
        bool delayedRestart();
        GwUpdate(GwLog *log,GwWebServer *webserver, PasswordChecker checker);
};