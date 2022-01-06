#pragma once
#include "GwBuffer.h"
class GwChannelInterface{
    public:
        virtual void loop(bool handleRead,bool handleWrite)=0;
        virtual void readMessages(GwMessageFetcher *writer)=0;
        virtual size_t sendToClients(const char *buffer, int sourceId, bool partial=false)=0;
        virtual Stream * getStream(bool partialWrites){ return NULL;}
};