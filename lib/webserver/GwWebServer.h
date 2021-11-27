#ifndef _GWWEBSERVER_H
#define _GWWEBSERVER_H
#include <ESPAsyncWebServer.h>
#include "GwMessage.h"
#include "GwLog.h"
class GwWebServer{
    private:
        AsyncWebServer *server;
        AsyncWebSocket *debugSocket;
        GwRequestQueue *queue;
        GwLog *logger;
    public:
        typedef GwRequestMessage *(RequestCreator)(AsyncWebServerRequest *request);
        GwWebServer(GwLog *logger, GwRequestQueue *queue,int port);
        ~GwWebServer();
        void begin();
        bool registerMainHandler(const char *url,RequestCreator creator);
        void handleAsyncWebRequest(AsyncWebServerRequest *request, GwRequestMessage *msg);
        void sendDebugLine(const char *line);
};
#endif