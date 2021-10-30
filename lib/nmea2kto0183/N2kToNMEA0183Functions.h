#ifndef _N2KTONMEA0183FUNCTIONS_H
#define _N2KTONMEA0183FUNCTIONS_H
/**
 * The class N2kToNMEA0183Functions is intended to be used for implementing all converter
 * functions
 * For easy extendability the implementation can be done completely within
 * this header file (java like) without the need to change multiple files
 */
#include <N2kMessages.h>
#include <NMEA0183Messages.h>
#include <math.h>
#include "N2kDataToNMEA0183.h"
#include <map>
#include "NMEA0183AISMessages.h"
#include "NMEA0183AISMsg.h"
class N2kToNMEA0183Functions : public N2kDataToNMEA0183
{
    typedef void (N2kToNMEA0183Functions::*N2KConverter)(const tN2kMsg &N2kMsg);

private:
    static const unsigned long RMCPeriod = 500;
    static void setMax(GwBoatItem<double> *maxItem, GwBoatItem<double> *item)
    {
        unsigned long now = millis();
        if (!item->isValid(now))
            return;
        if (item->getData() > maxItem->getData() || !maxItem->isValid(now))
        {
            maxItem->update(item->getData());
        }
    }
    static void updateDouble(GwBoatItem<double> *item, double value)
    {
        if (value == N2kDoubleNA)
            return;
        if (!item)
            return;
        item->update(value);
    }
    static double formatCourse(double cv)
    {
        double rt = cv * 180.0 / M_PI;
        if (rt > 360)
            rt -= 360;
        if (rt < 0)
            rt += 360;
        return rt;
    }
    static double formatWind(double cv)
    {
        double rt = formatCourse(cv);
        if (rt > 180)
            rt = 180 - rt;
        return rt;
    }
    static double formatKnots(double cv)
    {
        return cv * 3600.0 / 1852.0;
    }

    static uint32_t mtr2nm(uint32_t m)
    {
        return m / 1852;
    }
    GwBoatItem<double> *latitude;
    GwBoatItem<double> *longitude;
    GwBoatItem<double> *altitude;
    GwBoatItem<double> *variation;
    GwBoatItem<double> *heading;
    GwBoatItem<double> *cog;
    GwBoatItem<double> *sog;
    GwBoatItem<double> *stw;

    GwBoatItem<double> *tws;
    GwBoatItem<double> *twd;

    GwBoatItem<double> *aws;
    GwBoatItem<double> *awa;
    GwBoatItem<double> *awd;

    GwBoatItem<double> *maxAws;
    GwBoatItem<double> *maxTws;

    GwBoatItem<double> *rudderPosition;
    GwBoatItem<double> *waterTemperature;
    GwBoatItem<double> *waterDepth;

    GwBoatItem<uint32_t> *tripLog;
    GwBoatItem<uint32_t> *log;
    GwBoatItem<uint32_t> *daysSince1970;
    GwBoatItem<double> *secondsSinceMidnight;

    unsigned long LastPosSend;
    unsigned long NextRMCSend;
    unsigned long lastLoopTime;
    class ConverterEntry
    {
    public:
        unsigned long count = 0;
        N2KConverter converter;
        ConverterEntry(N2KConverter cv = NULL) { converter = cv; }
    };
    typedef std::map<long, ConverterEntry> ConverterMap;
    ConverterMap converters;

    /**
     *  register a n2k message converter
     *  each of the converter functions must be registered in the constructor 
     **/
    void registerConverter(long pgn, N2KConverter converter)
    {
        ConverterEntry e(converter);
        converters[pgn] = e;
    }
    virtual unsigned long *handledPgns()
    {
        logger->logString("CONV: # %d handled PGNS", (int)converters.size());
        unsigned long *rt = new unsigned long[converters.size() + 1];
        int idx = 0;
        for (ConverterMap::iterator it = converters.begin();
             it != converters.end(); it++)
        {
            rt[idx] = it->first;
            idx++;
        }
        rt[idx] = 0;
        return rt;
    }
    virtual void HandleMsg(const tN2kMsg &N2kMsg)
    {
        ConverterMap::iterator it;
        it = converters.find(N2kMsg.PGN);
        if (it != converters.end())
        {
            //logger->logString("CONV: handle PGN %ld",N2kMsg.PGN);
            (it->second).count++;
            //call to member function - see e.g. https://isocpp.org/wiki/faq/pointers-to-members
            ((*this).*((it->second).converter))(N2kMsg);
            return;
        }
    }
    virtual void toJson(JsonDocument &json)
    {
        for (ConverterMap::iterator it = converters.begin(); it != converters.end(); it++)
        {
            json["cnv"][String(it->first)] = it->second.count;
        }
        json["aisTargets"]=numShips();
    }
    virtual int numPgns()
    {
        return converters.size();
    }
    void SetNextRMCSend() { NextRMCSend = millis() + RMCPeriod; }

    //*************** the converters ***********************
    void HandleHeading(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        tN2kHeadingReference ref;
        double _Deviation = 0;
        double Variation;
        tNMEA0183Msg NMEA0183Msg;
        double Heading;
        if (ParseN2kHeading(N2kMsg, SID, Heading, _Deviation, Variation, ref))
        {
            if (ref == N2khr_magnetic)
            {
                updateDouble(variation, Variation); // Update Variation
                if (!N2kIsNA(Heading) && variation->isValid(millis()))
                    Heading -= variation->getData();
            }
            updateDouble(heading, Heading);
            if (NMEA0183SetHDG(NMEA0183Msg, heading->getDataWithDefault(NMEA0183DoubleNA), _Deviation, variation->getDataWithDefault(NMEA0183DoubleNA)))
            {
                SendMessage(NMEA0183Msg);
            }
        }
    }

    //*****************************************************************************
    void HandleVariation(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        tN2kMagneticVariation Source;
        uint16_t DaysSince1970;
        double Variation;
        ParseN2kMagneticVariation(N2kMsg, SID, Source, DaysSince1970, Variation);
        updateDouble(variation, Variation);
        if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
            daysSince1970->update(DaysSince1970);
    }

    //*****************************************************************************
    void HandleBoatSpeed(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        double WaterReferenced;
        double GroundReferenced;
        tN2kSpeedWaterReferenceType SWRT;

        if (ParseN2kBoatSpeed(N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT))
        {
            tNMEA0183Msg NMEA0183Msg;
            updateDouble(stw, WaterReferenced);
            unsigned long now = millis();
            double MagneticHeading = (heading->isValid(now) && variation->isValid(now)) ? heading->getData() + variation->getData() : NMEA0183DoubleNA;
            if (NMEA0183SetVHW(NMEA0183Msg, heading->getData(), MagneticHeading, WaterReferenced))
            {
                SendMessage(NMEA0183Msg);
            }
        }
    }

    //*****************************************************************************
    void HandleDepth(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        double DepthBelowTransducer;
        double Offset;
        double Range;
        double WaterDepth;
        if (ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range))
        {

            WaterDepth = DepthBelowTransducer + Offset;
            updateDouble(waterDepth, WaterDepth);
            tNMEA0183Msg NMEA0183Msg;
            if (NMEA0183SetDPT(NMEA0183Msg, DepthBelowTransducer, Offset))
            {
                SendMessage(NMEA0183Msg);
            }
            if (NMEA0183SetDBx(NMEA0183Msg, DepthBelowTransducer, Offset))
            {
                SendMessage(NMEA0183Msg);
            }
        }
    }

    //*****************************************************************************
    void HandlePosition(const tN2kMsg &N2kMsg)
    {
        double Latitude;
        double Longitude;
        if (ParseN2kPGN129025(N2kMsg, Latitude, Longitude))
        {
            updateDouble(latitude, Latitude);
            updateDouble(longitude, Longitude);
        }
    }

    //*****************************************************************************
    void HandleCOGSOG(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        tN2kHeadingReference HeadingReference;
        tNMEA0183Msg NMEA0183Msg;
        double COG;
        double SOG;
        if (ParseN2kCOGSOGRapid(N2kMsg, SID, HeadingReference, COG, SOG))
        {
            updateDouble(cog, COG);
            updateDouble(sog, SOG);
            double MCOG = (!N2kIsNA(COG) && variation->isValid()) ? COG - variation->getData() : NMEA0183DoubleNA;
            if (HeadingReference == N2khr_magnetic)
            {
                MCOG = COG;
                if (variation->isValid())
                {
                    COG -= variation->getData();
                    updateDouble(cog, COG);
                }
            }
            if (NMEA0183SetVTG(NMEA0183Msg, COG, MCOG, SOG))
            {
                SendMessage(NMEA0183Msg);
            }
        }
    }

    //*****************************************************************************
    void HandleGNSS(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        tN2kGNSStype GNSStype;
        tN2kGNSSmethod GNSSmethod;
        unsigned char nSatellites;
        double HDOP;
        double PDOP;
        double GeoidalSeparation;
        unsigned char nReferenceStations;
        tN2kGNSStype ReferenceStationType;
        uint16_t ReferenceSationID;
        double AgeOfCorrection;
        double Latitude;
        double Longitude;
        double Altitude;
        uint16_t DaysSince1970;
        double SecondsSinceMidnight;
        if (ParseN2kGNSS(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSStype, GNSSmethod,
                         nSatellites, HDOP, PDOP, GeoidalSeparation,
                         nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection))
        {
            updateDouble(latitude, Latitude);
            updateDouble(longitude, Longitude);
            updateDouble(altitude, Altitude);
            updateDouble(secondsSinceMidnight, SecondsSinceMidnight);
            if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
                daysSince1970->update(DaysSince1970);
        }
    }

    //*****************************************************************************
    void HandleWind(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        tN2kWindReference WindReference;
        tNMEA0183WindReference NMEA0183Reference = NMEA0183Wind_True;

        double x, y;
        double WindAngle, WindSpeed;

        if (ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference))
        {
            tNMEA0183Msg NMEA0183Msg;

            if (WindReference == N2kWind_Apparent)
            {
                NMEA0183Reference = NMEA0183Wind_Apparent;
                updateDouble(awa, WindAngle);
                updateDouble(aws, WindSpeed);
                setMax(maxAws, aws);
            }

            if (NMEA0183SetMWV(NMEA0183Msg, formatCourse(WindAngle), NMEA0183Reference, WindSpeed))
                SendMessage(NMEA0183Msg);

            if (WindReference == N2kWind_Apparent && sog->isValid())
            { // Lets calculate and send TWS/TWA if SOG is available

                x = WindSpeed * cos(WindAngle);
                y = WindSpeed * sin(WindAngle);

                updateDouble(twd, atan2(y, -sog->getData() + x));
                updateDouble(tws, sqrt((y * y) + ((-sog->getData() + x) * (-sog->getData() + x))));

                setMax(maxTws, tws);

                NMEA0183Reference = NMEA0183Wind_True;
                if (NMEA0183SetMWV(NMEA0183Msg,
                                   formatCourse(twd->getData()),
                                   NMEA0183Reference,
                                   tws->getDataWithDefault(NMEA0183DoubleNA)))
                    SendMessage(NMEA0183Msg);
                double magnetic = twd->getData();
                if (variation->isValid())
                    magnetic -= variation->getData();
                if (!NMEA0183Msg.Init("MWD", "GP"))
                    return;
                if (!NMEA0183Msg.AddDoubleField(formatCourse(twd->getData())))
                    return;
                if (!NMEA0183Msg.AddStrField("T"))
                    return;
                if (!NMEA0183Msg.AddDoubleField(formatCourse(magnetic)))
                    return;
                if (!NMEA0183Msg.AddStrField("M"))
                    return;
                if (!NMEA0183Msg.AddDoubleField(tws->getData() / 0.514444))
                    return;
                if (!NMEA0183Msg.AddStrField("N"))
                    return;
                if (!NMEA0183Msg.AddDoubleField(tws->getData()))
                    return;
                if (!NMEA0183Msg.AddStrField("M"))
                    return;

                SendMessage(NMEA0183Msg);
            }
        }
    }
    //*****************************************************************************
    void SendRMC()
    {
        long now = millis();
        if (NextRMCSend <= millis() && latitude->isValid(now))
        {
            tNMEA0183Msg NMEA0183Msg;
            if (NMEA0183SetRMC(NMEA0183Msg,

                               secondsSinceMidnight->getDataWithDefault(NMEA0183DoubleNA),
                               latitude->getDataWithDefault(NMEA0183DoubleNA),
                               longitude->getDataWithDefault(NMEA0183DoubleNA),
                               cog->getDataWithDefault(NMEA0183DoubleNA),
                               sog->getDataWithDefault(NMEA0183DoubleNA),
                               daysSince1970->getDataWithDefault(NMEA0183UInt32NA),
                               variation->getDataWithDefault(NMEA0183DoubleNA)))
            {
                SendMessage(NMEA0183Msg);
            }
            SetNextRMCSend();
        }
    }

    //*****************************************************************************
    void HandleLog(const tN2kMsg &N2kMsg)
    {
        uint16_t DaysSince1970;
        double SecondsSinceMidnight;
        uint32_t Log, TripLog;
        if (ParseN2kDistanceLog(N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog))
        {
            if (Log != N2kUInt32NA)
                log->update(Log);
            if (TripLog != N2kUInt32NA)
                tripLog->update(TripLog);
            if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
                daysSince1970->update(DaysSince1970);
            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("VLW", "GP"))
                return;
            if (!NMEA0183Msg.AddDoubleField(Log / 1852.0))
                return;
            if (!NMEA0183Msg.AddStrField("N"))
                return;
            if (!NMEA0183Msg.AddDoubleField(TripLog / 1852.0))
                return;
            if (!NMEA0183Msg.AddStrField("N"))
                return;

            SendMessage(NMEA0183Msg);
        }
    }

    //*****************************************************************************
    void HandleRudder(const tN2kMsg &N2kMsg)
    {

        unsigned char Instance;
        tN2kRudderDirectionOrder RudderDirectionOrder;
        double AngleOrder;
        double RudderPosition;

        if (ParseN2kRudder(N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder))
        {

            updateDouble(rudderPosition, RudderPosition);
            if (Instance != 0)
                return;

            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("RSA", "GP"))
                return;
            if (!NMEA0183Msg.AddDoubleField(formatCourse(RudderPosition)))
                return;
            if (!NMEA0183Msg.AddStrField("A"))
                return;
            if (!NMEA0183Msg.AddDoubleField(0.0))
                return;
            if (!NMEA0183Msg.AddStrField("A"))
                return;

            SendMessage(NMEA0183Msg);
        }
    }

    //*****************************************************************************
    void HandleWaterTemp(const tN2kMsg &N2kMsg)
    {

        unsigned char SID;
        double OutsideAmbientAirTemperature;
        double AtmosphericPressure;
        double WaterTemperature;
        if (ParseN2kPGN130310(N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure))
        {

            updateDouble(waterTemperature, WaterTemperature);
            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("MTW", "GP"))
                return;
            if (!NMEA0183Msg.AddDoubleField(KelvinToC(WaterTemperature)))
                return;
            if (!NMEA0183Msg.AddStrField("C"))
                return;

            SendMessage(NMEA0183Msg);
        }
    }

    //*****************************************************************************
    // 129038 AIS Class A Position Report (Message 1, 2, 3)
    void HandleAISClassAPosReport(const tN2kMsg &N2kMsg)
    {

        unsigned char SID;
        tN2kAISRepeat _Repeat;
        uint32_t _UserID; // MMSI
        double _Latitude;
        double _Longitude;
        bool _Accuracy;
        bool _RAIM;
        uint8_t _Seconds;
        double _COG;
        double _SOG;
        double _Heading;
        double _ROT;
        tN2kAISNavStatus _NavStatus;

        uint8_t _MessageType = 1;
        tNMEA0183AISMsg NMEA0183AISMsg;

        if (ParseN2kPGN129038(N2kMsg, SID, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy, _RAIM, _Seconds,
                              _COG, _SOG, _Heading, _ROT, _NavStatus))
        {

// Debug
#ifdef SERIAL_PRINT_AIS_FIELDS
            Serial.println("–––––––––––––––––––––––– Msg 1 ––––––––––––––––––––––––––––––––");

            const double pi = 3.1415926535897932384626433832795;
            const double radToDeg = 180.0 / pi;
            const double msTokn = 3600.0 / 1852.0;
            const double radsToDegMin = 60 * 360.0 / (2 * pi); // [rad/s -> degree/minute]
            Serial.print("Repeat: ");
            Serial.println(_Repeat);
            Serial.print("UserID: ");
            Serial.println(_UserID);
            Serial.print("Latitude: ");
            Serial.println(_Latitude);
            Serial.print("Longitude: ");
            Serial.println(_Longitude);
            Serial.print("Accuracy: ");
            Serial.println(_Accuracy);
            Serial.print("RAIM: ");
            Serial.println(_RAIM);
            Serial.print("Seconds: ");
            Serial.println(_Seconds);
            Serial.print("COG: ");
            Serial.println(_COG * radToDeg);
            Serial.print("SOG: ");
            Serial.println(_SOG * msTokn);
            Serial.print("Heading: ");
            Serial.println(_Heading * radToDeg);
            Serial.print("ROT: ");
            Serial.println(_ROT * radsToDegMin);
            Serial.print("NavStatus: ");
            Serial.println(_NavStatus);
#endif

            if (SetAISClassABMessage1(NMEA0183AISMsg, _MessageType, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy,
                                      _RAIM, _Seconds, _COG, _SOG, _Heading, _ROT, _NavStatus))
            {

                SendMessage(NMEA0183AISMsg);

#ifdef SERIAL_PRINT_AIS_NMEA
                // Debug Print AIS-NMEA
                Serial.print(NMEA0183AISMsg.GetPrefix());
                Serial.print(NMEA0183AISMsg.Sender());
                Serial.print(NMEA0183AISMsg.MessageCode());
                for (int i = 0; i < NMEA0183AISMsg.FieldCount(); i++)
                {
                    Serial.print(",");
                    Serial.print(NMEA0183AISMsg.Field(i));
                }
                char buf[7];
                sprintf(buf, "*%02X\r\n", NMEA0183AISMsg.GetCheckSum());
                Serial.print(buf);
#endif
            }
        }
    } // end 129038 AIS Class A Position Report Message 1/3

    //*****************************************************************************
    // 129039 AIS Class B Position Report -> AIS Message Type 5: Static and Voyage Related Data
    void HandleAISClassAMessage5(const tN2kMsg &N2kMsg)
    {
        uint8_t _MessageID;
        tN2kAISRepeat _Repeat;
        uint32_t _UserID; // MMSI
        uint32_t _IMONumber;
        char _Callsign[8];
        char _Name[21];
        uint8_t _VesselType;
        double _Length;
        double _Beam;
        double _PosRefStbd;
        double _PosRefBow;
        uint16_t _ETAdate;
        double _ETAtime;
        double _Draught;
        char _Destination[21];
        tN2kAISVersion _AISversion;
        tN2kGNSStype _GNSStype;
        tN2kAISTranceiverInfo _AISinfo;
        tN2kAISDTE _DTE;

        tNMEA0183AISMsg NMEA0183AISMsg;

        if (ParseN2kPGN129794(N2kMsg, _MessageID, _Repeat, _UserID, _IMONumber, _Callsign, _Name, _VesselType,
                              _Length, _Beam, _PosRefStbd, _PosRefBow, _ETAdate, _ETAtime, _Draught, _Destination,
                              _AISversion, _GNSStype, _DTE, _AISinfo))
        {

#ifdef SERIAL_PRINT_AIS_FIELDS
            // Debug Print N2k Values
            Serial.println("––––––––––––––––––––––– Msg 5 –––––––––––––––––––––––––––––––––");
            Serial.print("MessageID: ");
            Serial.println(_MessageID);
            Serial.print("Repeat: ");
            Serial.println(_Repeat);
            Serial.print("UserID: ");
            Serial.println(_UserID);
            Serial.print("IMONumber: ");
            Serial.println(_IMONumber);
            Serial.print("Callsign: ");
            Serial.println(_Callsign);
            Serial.print("VesselType: ");
            Serial.println(_VesselType);
            Serial.print("Name: ");
            Serial.println(_Name);
            Serial.print("Length: ");
            Serial.println(_Length);
            Serial.print("Beam: ");
            Serial.println(_Beam);
            Serial.print("PosRefStbd: ");
            Serial.println(_PosRefStbd);
            Serial.print("PosRefBow: ");
            Serial.println(_PosRefBow);
            Serial.print("ETAdate: ");
            Serial.println(_ETAdate);
            Serial.print("ETAtime: ");
            Serial.println(_ETAtime);
            Serial.print("Draught: ");
            Serial.println(_Draught);
            Serial.print("Destination: ");
            Serial.println(_Destination);
            Serial.print("GNSStype: ");
            Serial.println(_GNSStype);
            Serial.print("DTE: ");
            Serial.println(_DTE);
            Serial.println("––––––––––––––––––––––– Msg 5 –––––––––––––––––––––––––––––––––");
#endif

            if (SetAISClassAMessage5(NMEA0183AISMsg, _MessageID, _Repeat, _UserID, _IMONumber, _Callsign, _Name, _VesselType,
                                     _Length, _Beam, _PosRefStbd, _PosRefBow, _ETAdate, _ETAtime, _Draught, _Destination,
                                     _GNSStype, _DTE))
            {

                SendMessage(NMEA0183AISMsg.BuildMsg5Part1(NMEA0183AISMsg));

#ifdef SERIAL_PRINT_AIS_NMEA
                // Debug Print AIS-NMEA Message Type 5, Part 1
                char buf[7];
                Serial.print(NMEA0183AISMsg.GetPrefix());
                Serial.print(NMEA0183AISMsg.Sender());
                Serial.print(NMEA0183AISMsg.MessageCode());
                for (int i = 0; i < NMEA0183AISMsg.FieldCount(); i++)
                {
                    Serial.print(",");
                    Serial.print(NMEA0183AISMsg.Field(i));
                }
                sprintf(buf, "*%02X\r\n", NMEA0183AISMsg.GetCheckSum());
                Serial.print(buf);
#endif

                SendMessage(NMEA0183AISMsg.BuildMsg5Part2(NMEA0183AISMsg));

#ifdef SERIAL_PRINT_AIS_NMEA
                // Print AIS-NMEA Message Type 5, Part 2
                Serial.print(NMEA0183AISMsg.GetPrefix());
                Serial.print(NMEA0183AISMsg.Sender());
                Serial.print(NMEA0183AISMsg.MessageCode());
                for (int i = 0; i < NMEA0183AISMsg.FieldCount(); i++)
                {
                    Serial.print(",");
                    Serial.print(NMEA0183AISMsg.Field(i));
                }
                sprintf(buf, "*%02X\r\n", NMEA0183AISMsg.GetCheckSum());
                Serial.print(buf);
#endif
            }
        }
    }

    //
    //*****************************************************************************
    // 129039 AIS Class B Position Report (Message 18)
    void HandleAISClassBMessage18(const tN2kMsg &N2kMsg)
    {
        uint8_t _MessageID;
        tN2kAISRepeat _Repeat;
        uint32_t _UserID; // MMSI
        double _Latitude;
        double _Longitude;
        bool _Accuracy;
        bool _RAIM;
        uint8_t _Seconds;
        double _COG;
        double _SOG;
        double _Heading;
        tN2kAISUnit _Unit;
        bool _Display, _DSC, _Band, _Msg22, _State;
        tN2kAISMode _Mode;

        if (ParseN2kPGN129039(N2kMsg, _MessageID, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy, _RAIM,
                              _Seconds, _COG, _SOG, _Heading, _Unit, _Display, _DSC, _Band, _Msg22, _Mode, _State))
        {

            tNMEA0183AISMsg NMEA0183AISMsg;

            if (SetAISClassBMessage18(NMEA0183AISMsg, _MessageID, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy, _RAIM,
                                      _Seconds, _COG, _SOG, _Heading, _Unit, _Display, _DSC, _Band, _Msg22, _Mode, _State))
            {

                SendMessage(NMEA0183AISMsg);

#ifdef SERIAL_PRINT_AIS_NMEA
                // Debug Print AIS-NMEA
                Serial.print(NMEA0183AISMsg.GetPrefix());
                Serial.print(NMEA0183AISMsg.Sender());
                Serial.print(NMEA0183AISMsg.MessageCode());
                for (int i = 0; i < NMEA0183AISMsg.FieldCount(); i++)
                {
                    Serial.print(",");
                    Serial.print(NMEA0183AISMsg.Field(i));
                }
                char buf[7];
                sprintf(buf, "*%02X\r\n", NMEA0183AISMsg.GetCheckSum());
                Serial.print(buf);
#endif
            }
        }
        return;
    }

    //*****************************************************************************
    // PGN 129809 AIS Class B "CS" Static Data Report, Part A
    void HandleAISClassBMessage24A(const tN2kMsg &N2kMsg)
    {

        uint8_t _MessageID;
        tN2kAISRepeat _Repeat;
        uint32_t _UserID; // MMSI
        char _Name[21];

        if (ParseN2kPGN129809(N2kMsg, _MessageID, _Repeat, _UserID, _Name))
        {

            tNMEA0183AISMsg NMEA0183AISMsg;
            if (SetAISClassBMessage24PartA(NMEA0183AISMsg, _MessageID, _Repeat, _UserID, _Name))
            {
            }
        }
        return;
    }

    //*****************************************************************************
    // PGN 129810 AIS Class B "CS" Static Data Report, Part B -> AIS Message 24 (2 Parts)
    void HandleAISClassBMessage24B(const tN2kMsg &N2kMsg)
    {

        uint8_t _MessageID;
        tN2kAISRepeat _Repeat;
        uint32_t _UserID, _MothershipID; // MMSI
        char _Callsign[8];
        char _Vendor[4];
        uint8_t _VesselType;
        double _Length;
        double _Beam;
        double _PosRefStbd;
        double _PosRefBow;

        if (ParseN2kPGN129810(N2kMsg, _MessageID, _Repeat, _UserID, _VesselType, _Vendor, _Callsign,
                              _Length, _Beam, _PosRefStbd, _PosRefBow, _MothershipID))
        {

//
#ifdef SERIAL_PRINT_AIS_FIELDS
            // Debug Print N2k Values
            Serial.println("––––––––––––––––––––––– Msg 24 ––––––––––––––––––––––––––––––––");
            Serial.print("MessageID: ");
            Serial.println(_MessageID);
            Serial.print("Repeat: ");
            Serial.println(_Repeat);
            Serial.print("UserID: ");
            Serial.println(_UserID);
            Serial.print("VesselType: ");
            Serial.println(_VesselType);
            Serial.print("Vendor: ");
            Serial.println(_Vendor);
            Serial.print("Callsign: ");
            Serial.println(_Callsign);
            Serial.print("Length: ");
            Serial.println(_Length);
            Serial.print("Beam: ");
            Serial.println(_Beam);
            Serial.print("PosRefStbd: ");
            Serial.println(_PosRefStbd);
            Serial.print("PosRefBow: ");
            Serial.println(_PosRefBow);
            Serial.print("MothershipID: ");
            Serial.println(_MothershipID);
            Serial.println("––––––––––––––––––––––– Msg 24 ––––––––––––––––––––––––––––––––");
#endif

            tNMEA0183AISMsg NMEA0183AISMsg;

            if (SetAISClassBMessage24(NMEA0183AISMsg, _MessageID, _Repeat, _UserID, _VesselType, _Vendor, _Callsign,
                                      _Length, _Beam, _PosRefStbd, _PosRefBow, _MothershipID))
            {

                SendMessage(NMEA0183AISMsg.BuildMsg24PartA(NMEA0183AISMsg));

#ifdef SERIAL_PRINT_AIS_NMEA
                // Debug Print AIS-NMEA
                char buf[7];
                Serial.print(NMEA0183AISMsg.GetPrefix());
                Serial.print(NMEA0183AISMsg.Sender());
                Serial.print(NMEA0183AISMsg.MessageCode());
                for (int i = 0; i < NMEA0183AISMsg.FieldCount(); i++)
                {
                    Serial.print(",");
                    Serial.print(NMEA0183AISMsg.Field(i));
                }
                sprintf(buf, "*%02X\r\n", NMEA0183AISMsg.GetCheckSum());
                Serial.print(buf);
#endif

                SendMessage(NMEA0183AISMsg.BuildMsg24PartB(NMEA0183AISMsg));

#ifdef SERIAL_PRINT_AIS_NMEA
                Serial.print(NMEA0183AISMsg.GetPrefix());
                Serial.print(NMEA0183AISMsg.Sender());
                Serial.print(NMEA0183AISMsg.MessageCode());
                for (int i = 0; i < NMEA0183AISMsg.FieldCount(); i++)
                {
                    Serial.print(",");
                    Serial.print(NMEA0183AISMsg.Field(i));
                }
                sprintf(buf, "*%02X\r\n", NMEA0183AISMsg.GetCheckSum());
                Serial.print(buf);
#endif
            }
        }
        return;
    }

public:
    N2kToNMEA0183Functions(GwLog *logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183, int sourceId) 
    : N2kDataToNMEA0183(logger, boatData, NMEA2000, NMEA0183,sourceId)
    {
        LastPosSend = 0;
        lastLoopTime = 0;
        NextRMCSend = millis() + RMCPeriod;

        this->logger = logger;
        this->boatData = boatData;
        heading = boatData->getDoubleItem(F("Heading"), true, 4000, &formatCourse);
        latitude = boatData->getDoubleItem(F("Latitude"), true, 4000);
        longitude = boatData->getDoubleItem(F("Longitude"), true, 4000);
        altitude = boatData->getDoubleItem(F("Altitude"), true, 4000);
        cog = boatData->getDoubleItem(F("COG"), true, 4000, &formatCourse);
        sog = boatData->getDoubleItem(F("SOG"), true, 4000, &formatKnots);
        stw = boatData->getDoubleItem(F("STW"), true, 4000, &formatKnots);
        variation = boatData->getDoubleItem(F("Variation"), true, 4000, &formatCourse);
        tws = boatData->getDoubleItem(F("TWS"), true, 4000, &formatKnots);
        twd = boatData->getDoubleItem(F("TWD"), true, 4000, &formatCourse);
        aws = boatData->getDoubleItem(F("AWS"), true, 4000, &formatKnots);
        awa = boatData->getDoubleItem(F("AWA"), true, 4000, &formatWind);
        awd = boatData->getDoubleItem(F("AWD"), true, 4000, &formatCourse);
        maxAws = boatData->getDoubleItem(F("MaxAws"), true, 0, &formatKnots);
        maxTws = boatData->getDoubleItem(F("MaxTws"), true, 0, &formatKnots);
        rudderPosition = boatData->getDoubleItem(F("RudderPosition"), true, 4000, &formatCourse);
        waterTemperature = boatData->getDoubleItem(F("WaterTemperature"), true, 4000, &KelvinToC);
        waterDepth = boatData->getDoubleItem(F("WaterDepth"), true, 4000);
        log = boatData->getUint32Item(F("Log"), true, 0, mtr2nm);
        tripLog = boatData->getUint32Item(F("TripLog"), true, 0, mtr2nm);
        daysSince1970 = boatData->getUint32Item(F("DaysSince1970"), true, 4000);
        secondsSinceMidnight = boatData->getDoubleItem(F("SecondsSinceMidnight"), true, 4000);

        //register all converter functions
        //for each vonverter you should have a member with the N2KMsg as parameter
        //and register it here
        //with this approach we easily have a list of all handled
        //pgns
        registerConverter(127250UL, &N2kToNMEA0183Functions::HandleHeading);
        registerConverter(127258UL, &N2kToNMEA0183Functions::HandleVariation);
        registerConverter(128259UL, &N2kToNMEA0183Functions::HandleBoatSpeed);
        registerConverter(128267UL, &N2kToNMEA0183Functions::HandleDepth);
        registerConverter(129025UL, &N2kToNMEA0183Functions::HandlePosition);
        registerConverter(129026UL, &N2kToNMEA0183Functions::HandleCOGSOG);
        registerConverter(129029UL, &N2kToNMEA0183Functions::HandleGNSS);
        registerConverter(130306UL, &N2kToNMEA0183Functions::HandleWind);
        registerConverter(128275UL, &N2kToNMEA0183Functions::HandleLog);
        registerConverter(127245UL, &N2kToNMEA0183Functions::HandleRudder);
        registerConverter(130310UL, &N2kToNMEA0183Functions::HandleWaterTemp);
#define HANDLE_AIS         
#ifdef HANDLE_AIS
        registerConverter(129038UL, &N2kToNMEA0183Functions::HandleAISClassAPosReport);  // AIS Class A Position Report, Message Type 1
        registerConverter(129039UL, &N2kToNMEA0183Functions::HandleAISClassBMessage18);  // AIS Class B Position Report, Message Type 18
        registerConverter(129794UL, &N2kToNMEA0183Functions::HandleAISClassAMessage5);   // AIS Class A Ship Static and Voyage related data, Message Type 5
        registerConverter(129809UL, &N2kToNMEA0183Functions::HandleAISClassBMessage24A); // AIS Class B "CS" Static Data Report, Part A
        registerConverter(129810UL, &N2kToNMEA0183Functions::HandleAISClassBMessage24B); // AIS Class B "CS" Static Data Report, Part B
#endif       

    }
    virtual void loop()
    {
        N2kDataToNMEA0183::loop();
        unsigned long now = millis();
        if (now < (lastLoopTime + 100))
            return;
        lastLoopTime = now;
        SendRMC();
    }
};
#endif