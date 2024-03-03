#ifndef _NMEA0183DATATON2K_H
#define _NMEA0183DATATON2K_H
#include "GwLog.h"
#include "GwBoatData.h"
#include "N2kMessages.h"
#include "GwXDRMappings.h"
#include "GwConverterConfig.h"

class NMEA0183DataToN2K{
    public:
        typedef bool (*N2kSender)(const tN2kMsg &msg,int sourceId);
    protected:
        GwLog * logger;
        GwBoatData *boatData;
        N2kSender sender;
        GwConverterConfig config;
        unsigned long lastRmc=millis();
    public:
        NMEA0183DataToN2K(GwLog *logger,GwBoatData *boatData,N2kSender callback);
        virtual bool parseAndSend(const char *buffer, int sourceId)=0;
        virtual unsigned long *handledPgns()=0;
        virtual int numConverters()=0;
        virtual String handledKeys()=0;
        unsigned long getLastRmc()const {return lastRmc; }
        static NMEA0183DataToN2K* create(GwLog *logger,GwBoatData *boatData,N2kSender callback,
            GwXDRMappings *xdrMappings,
            const GwConverterConfig &config);
};
#endif