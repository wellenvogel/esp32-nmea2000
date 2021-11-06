#ifndef _GWAPI_H
#define _GWAPI_H
#include "GwMessage.h"
#include "N2kMessages.h"
#include "NMEA0183Messages.h"
#include "GWConfig.h"
//API to be used for additional tasks
class GwApi{
    public:
        virtual GwRequestQueue *getQueue()=0;
        virtual void sendN2kMessage(const tN2kMsg &msg)=0;
        virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, int sourceId)=0;
        virtual int getSourceId()=0;
        virtual GwConfigHandler *getConfig()=0;
        virtual GwLog *getLogger()=0;
};
#endif