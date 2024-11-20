#ifndef _GWWEBSERVER_H
#define _GWWEBSERVER_H
#include <ESPAsyncWebServer.h>
#include <functional>
#include "GwMessage.h"
#include "GwLog.h"
#include "GwApi.h"
class GwWebServer{
    private:
        AsyncWebServer *server;
        GwRequestQueue *queue;
        GwLog *logger;
    public:
        typedef GwRequestMessage *(RequestCreator)(AsyncWebServerRequest *request);
        using HandlerFunction=GwApi::HandlerFunction;
        GwWebServer(GwLog *logger, GwRequestQueue *queue,int port);
        ~GwWebServer();
        void begin();
        bool registerMainHandler(const char *url,RequestCreator creator);
        bool registerHandler(const char * url,HandlerFunction handler);
        bool registerPostHandler(const char *url, ArRequestHandlerFunction requestHandler, ArBodyHandlerFunction bodyHandler);
        void handleAsyncWebRequest(AsyncWebServerRequest *request, GwRequestMessage *msg);
        AsyncWebServer * getServer(){return server;}
};
#endif