#ifndef _GWWEBSERVER_H
#define _GWWEBSERVER_H
#include <ESPAsyncWebServer.h>
#include "GwMessage.h"
#include "GwLog.h"
class GwWebServer{
    private:
        AsyncWebServer *server;
        QueueHandle_t queue;
        GwLog *logger;
    public:
        typedef RequestMessage *(RequestCreator)(AsyncWebServerRequest *request);
        GwWebServer(GwLog *logger, int port);
        ~GwWebServer();
        void begin();
        bool registerMainHandler(const char *url,RequestCreator creator);
        void fetchMainRequest(); //to be called from main loop
        void handleAsyncWebRequest(AsyncWebServerRequest *request, RequestMessage *msg);
};
#endif