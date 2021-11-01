#ifndef _NMEA0183DATATON2K_H
#define _NMEA0183DATATON2K_H
#include "GwLog.h"
#include "GwBoatData.h"
#include "N2kMessages.h"

class NMEA0183DataToN2K{
    public:
        typedef bool (*N2kSender)(const tN2kMsg &msg);
    protected:
        GwLog * logger;
        GwBoatData *boatData;
        N2kSender sender;
    public:
        NMEA0183DataToN2K(GwLog *logger,GwBoatData *boatData,N2kSender callback);
        virtual bool parseAndSend(const char *buffer, int sourceId);
        virtual unsigned long *handledPgns()=0;
        virtual int numConverters()=0;
        static NMEA0183DataToN2K* create(GwLog *logger,GwBoatData *boatData,N2kSender callback);
};
#endif