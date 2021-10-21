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
class N2kToNMEA0183Functions : public N2kDataToNMEA0183
{
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

public:
    N2kToNMEA0183Functions(GwLog *logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183) : N2kDataToNMEA0183(logger, boatData, NMEA2000, NMEA0183)
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
    virtual void HandleMsg(const tN2kMsg &N2kMsg)
    {
        switch (N2kMsg.PGN)
        {
        case 127250UL:
            HandleHeading(N2kMsg);
        case 127258UL:
            HandleVariation(N2kMsg);
        case 128259UL:
            HandleBoatSpeed(N2kMsg);
        case 128267UL:
            HandleDepth(N2kMsg);
        case 129025UL:
            HandlePosition(N2kMsg);
        case 129026UL:
            HandleCOGSOG(N2kMsg);
        case 129029UL:
            HandleGNSS(N2kMsg);
        case 130306UL:
            HandleWind(N2kMsg);
        case 128275UL:
            HandleLog(N2kMsg);
        case 127245UL:
            HandleRudder(N2kMsg);
        case 130310UL:
            HandleWaterTemp(N2kMsg);
        }
    }

private:
};
#endif