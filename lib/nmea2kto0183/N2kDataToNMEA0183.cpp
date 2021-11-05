/*
  N2kDataToNMEA0183.cpp

  Copyright (c) 2015-2018 Timo Lappalainen, Kave Oy, www.kave.fi

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to use,
  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
  Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
  OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <N2kMessages.h>
#include <NMEA0183Messages.h>
#include <math.h>

#include "N2kDataToNMEA0183.h"
#include "NMEA0183AISMessages.h"
#include "ConverterList.h"




N2kDataToNMEA0183::N2kDataToNMEA0183(GwLog * logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, 
  tSendNMEA0183MessageCallback callback, int id,String talkerId) 
: tNMEA2000::tMsgHandler(0,NMEA2000){
    SendNMEA0183MessageCallback=0;
    this->SendNMEA0183MessageCallback=callback;
    strncpy(this->talkerId,talkerId.c_str(),2);
    talkerId[2]=0;
    sourceId=id;   
  }



//*****************************************************************************
void N2kDataToNMEA0183::loop() {
}

//*****************************************************************************
void N2kDataToNMEA0183::SendMessage(const tNMEA0183Msg &NMEA0183Msg) {
  if ( SendNMEA0183MessageCallback != 0 ) SendNMEA0183MessageCallback(NMEA0183Msg, sourceId);
}

/**
 * The class N2kToNMEA0183Functions is intended to be used for implementing all converter
 * functions
 * For easy extendability the implementation can be done completely within
 * this header file (java like) without the need to change multiple files
 */

class N2kToNMEA0183Functions : public N2kDataToNMEA0183
{

private:
    ConverterList<N2kToNMEA0183Functions,tN2kMsg> converters;
    static const unsigned long RMCPeriod = 500;
    void setMax(GwBoatItem<double> *maxItem, GwBoatItem<double> *item)
    {
        unsigned long now = millis();
        if (!item->isValid(now))
            return;
        if (item->getData() > maxItem->getData() || !maxItem->isValid(now))
        {
            maxItem->update(item->getData(),sourceId);
        }
    }
    void updateDouble(GwBoatItem<double> *item, double value)
    {
        if (value == N2kDoubleNA)
            return;
        if (!item)
            return;
        item->update(value,sourceId);
    }
    
    unsigned long LastPosSend;
    unsigned long NextRMCSend;
    unsigned long lastLoopTime;
    
    virtual unsigned long *handledPgns()
    {
        logger->logString("CONV: # %d handled PGNS", converters.numConverters());
        return converters.handledPgns();
    }
    virtual String handledKeys(){
        return converters.handledKeys();
    }
    virtual void HandleMsg(const tN2kMsg &N2kMsg)
    {
        String key=String(N2kMsg.PGN);
        bool rt=converters.handleMessage(key,N2kMsg,this);
        if (! rt){
          LOG_DEBUG(GwLog::DEBUG+1,"no handler for %ld",N2kMsg.PGN);
        }
    }
    virtual void toJson(JsonDocument &json)
    {
        converters.toJson(String(F("cnv")),json);
    }
    virtual int numPgns()
    {
        return converters.numConverters();
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
        //if we have heading and variation we send HDG (mag.) and HDT
        //if we have no variation we send either HDM or HDT
        if (ParseN2kHeading(N2kMsg, SID, Heading, _Deviation, Variation, ref))
        {
            updateDouble(boatData->Variation,Variation);
            updateDouble(boatData->Deviation,_Deviation);
            if (N2kIsNA(Variation)){
                //maybe we still have a valid variation
                Variation=boatData->Variation->getDataWithDefault(N2kDoubleNA);
            }
            if (N2kIsNA(Variation)){
                //no variation
                if (ref == N2khr_magnetic){
                    updateDouble(boatData->MagneticHeading,Heading);
                    if (NMEA0183SetHDM(NMEA0183Msg,Heading,talkerId)){
                        SendMessage(NMEA0183Msg);
                    }    
                }
                if (ref == N2khr_true){
                    updateDouble(boatData->Heading,Heading);
                    if (NMEA0183SetHDT(NMEA0183Msg,Heading,talkerId)){
                        SendMessage(NMEA0183Msg);
                    }
                }
            }
            else{
                double MagneticHeading=N2kDoubleNA;
                if (ref == N2khr_magnetic){
                    MagneticHeading=Heading;
                    Heading+=Variation;
                }
                if (ref == N2khr_true){
                    MagneticHeading=Heading-Variation;
                }
                updateDouble(boatData->MagneticHeading,MagneticHeading);
                updateDouble(boatData->Heading,Heading);
                if (!N2kIsNA(MagneticHeading)){
                    if (NMEA0183SetHDG(NMEA0183Msg, MagneticHeading,_Deviation, 
                        Variation,talkerId))
                    {
                        SendMessage(NMEA0183Msg);
                    }
                }
                if (!N2kIsNA(Heading)){
                    if (NMEA0183SetHDT(NMEA0183Msg, Heading,talkerId))
                    {
                        SendMessage(NMEA0183Msg);
                    }
                }
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
        updateDouble(boatData->Variation, Variation);
        if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
            boatData->DaysSince1970->update(DaysSince1970,sourceId);
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
            updateDouble(boatData->STW, WaterReferenced);
            unsigned long now = millis();
            double MagneticHeading = (boatData->Heading->isValid(now) && boatData->Variation->isValid(now)) ? boatData->Heading->getData() + boatData->Variation->getData() : NMEA0183DoubleNA;
            if (NMEA0183SetVHW(NMEA0183Msg, boatData->Heading->getData(), MagneticHeading, WaterReferenced,talkerId))
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
            updateDouble(boatData->WaterDepth, WaterDepth);
            tNMEA0183Msg NMEA0183Msg;
            if (NMEA0183SetDPT(NMEA0183Msg, DepthBelowTransducer, Offset,talkerId))
            {
                SendMessage(NMEA0183Msg);
            }
            if (NMEA0183SetDBx(NMEA0183Msg, DepthBelowTransducer, Offset,talkerId))
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
            updateDouble(boatData->Latitude, Latitude);
            updateDouble(boatData->Longitude, Longitude);
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
            updateDouble(boatData->COG, COG);
            updateDouble(boatData->SOG, SOG);
            double MCOG = (!N2kIsNA(COG) && boatData->Variation->isValid()) ? COG - boatData->Variation->getData() : NMEA0183DoubleNA;
            if (HeadingReference == N2khr_magnetic)
            {
                MCOG = COG;
                if (boatData->Variation->isValid())
                {
                    COG -= boatData->Variation->getData();
                    updateDouble(boatData->COG, COG);
                }
            }
            if (NMEA0183SetVTG(NMEA0183Msg, COG, MCOG, SOG,talkerId))
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
            updateDouble(boatData->Latitude, Latitude);
            updateDouble(boatData->Longitude, Longitude);
            updateDouble(boatData->Altitude, Altitude);
            updateDouble(boatData->SecondsSinceMidnight, SecondsSinceMidnight);
            if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
                boatData->DaysSince1970->update(DaysSince1970,sourceId);
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
                updateDouble(boatData->AWA, WindAngle);
                updateDouble(boatData->AWS, WindSpeed);
                setMax(boatData->MaxAws, boatData->AWS);
            }
            if (WindReference == N2kWind_True_North)
            {
                NMEA0183Reference = NMEA0183Wind_True;
                updateDouble(boatData->TWD, WindAngle);
                updateDouble(boatData->TWS, WindSpeed);
            }

            if (NMEA0183SetMWV(NMEA0183Msg, formatCourse(WindAngle), NMEA0183Reference, WindSpeed))
                SendMessage(NMEA0183Msg);

            if (WindReference == N2kWind_Apparent && boatData->SOG->isValid())
            { // Lets calculate and send TWS/TWA if SOG is available

                x = WindSpeed * cos(WindAngle);
                y = WindSpeed * sin(WindAngle);

                updateDouble(boatData->TWD, atan2(y, -boatData->SOG->getData() + x));
                updateDouble(boatData->TWS, sqrt((y * y) + ((-boatData->SOG->getData() + x) * (-boatData->SOG->getData() + x))));

                setMax(boatData->MaxTws, boatData->TWS);

                NMEA0183Reference = NMEA0183Wind_True;
                if (NMEA0183SetMWV(NMEA0183Msg,
                                   formatCourse(boatData->TWD->getData()),
                                   NMEA0183Reference,
                                   boatData->TWS->getDataWithDefault(NMEA0183DoubleNA),talkerId))
                    SendMessage(NMEA0183Msg);
                double magnetic = boatData->TWD->getData();
                if (boatData->Variation->isValid())
                    magnetic -= boatData->Variation->getData();
                if (!NMEA0183Msg.Init("MWD", talkerId))
                    return;
                if (!NMEA0183Msg.AddDoubleField(formatCourse(boatData->TWD->getData())))
                    return;
                if (!NMEA0183Msg.AddStrField("T"))
                    return;
                if (!NMEA0183Msg.AddDoubleField(formatCourse(magnetic)))
                    return;
                if (!NMEA0183Msg.AddStrField("M"))
                    return;
                if (!NMEA0183Msg.AddDoubleField(boatData->TWS->getData() / 0.514444))
                    return;
                if (!NMEA0183Msg.AddStrField("N"))
                    return;
                if (!NMEA0183Msg.AddDoubleField(boatData->TWS->getData()))
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
        if (NextRMCSend <= millis() && 
          boatData->Latitude->isValid(now) && 
          boatData->Latitude->getLastSource() == sourceId
          )
        {
            tNMEA0183Msg NMEA0183Msg;
            if (NMEA0183SetRMC(NMEA0183Msg,

                               boatData->SecondsSinceMidnight->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->Latitude->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->Longitude->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->COG->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->SOG->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->DaysSince1970->getDataWithDefault(NMEA0183UInt32NA),
                               boatData->Variation->getDataWithDefault(NMEA0183DoubleNA),
                               talkerId))
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
                boatData->Log->update(Log,sourceId);
            if (TripLog != N2kUInt32NA)
                boatData->TripLog->update(TripLog,sourceId);
            if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
                boatData->DaysSince1970->update(DaysSince1970,sourceId);
            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("VLW", talkerId))
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

            updateDouble(boatData->RudderPosition, RudderPosition);
            if (Instance != 0)
                return;

            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("RSA", talkerId))
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

            updateDouble(boatData->WaterTemperature, WaterTemperature);
            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("MTW", talkerId))
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


    void registerConverters()
    {
      //register all converter functions
      //for each vonverter you should have a member with the N2KMsg as parameter
      //and register it here
      //with this approach we easily have a list of all handled
      //pgns
      converters.registerConverter(127250UL, &N2kToNMEA0183Functions::HandleHeading);
      converters.registerConverter(127258UL, &N2kToNMEA0183Functions::HandleVariation);
      converters.registerConverter(128259UL, &N2kToNMEA0183Functions::HandleBoatSpeed);
      converters.registerConverter(128267UL, &N2kToNMEA0183Functions::HandleDepth);
      converters.registerConverter(129025UL, &N2kToNMEA0183Functions::HandlePosition);
      converters.registerConverter(129026UL, &N2kToNMEA0183Functions::HandleCOGSOG);
      converters.registerConverter(129029UL, &N2kToNMEA0183Functions::HandleGNSS);
      converters.registerConverter(130306UL, &N2kToNMEA0183Functions::HandleWind);
      converters.registerConverter(128275UL, &N2kToNMEA0183Functions::HandleLog);
      converters.registerConverter(127245UL, &N2kToNMEA0183Functions::HandleRudder);
      converters.registerConverter(130310UL, &N2kToNMEA0183Functions::HandleWaterTemp);
#define HANDLE_AIS
#ifdef HANDLE_AIS
      converters.registerConverter(129038UL, &N2kToNMEA0183Functions::HandleAISClassAPosReport);  // AIS Class A Position Report, Message Type 1
      converters.registerConverter(129039UL, &N2kToNMEA0183Functions::HandleAISClassBMessage18);  // AIS Class B Position Report, Message Type 18
      converters.registerConverter(129794UL, &N2kToNMEA0183Functions::HandleAISClassAMessage5);   // AIS Class A Ship Static and Voyage related data, Message Type 5
      converters.registerConverter(129809UL, &N2kToNMEA0183Functions::HandleAISClassBMessage24A); // AIS Class B "CS" Static Data Report, Part A
      converters.registerConverter(129810UL, &N2kToNMEA0183Functions::HandleAISClassBMessage24B); // AIS Class B "CS" Static Data Report, Part B
#endif
    }

  public:
    N2kToNMEA0183Functions(GwLog *logger, GwBoatData *boatData, 
        tNMEA2000 *NMEA2000, tSendNMEA0183MessageCallback callback, int sourceId,
        String talkerId) 
    : N2kDataToNMEA0183(logger, boatData, NMEA2000, callback,sourceId,talkerId)
    {
        LastPosSend = 0;
        lastLoopTime = 0;
        NextRMCSend = millis() + RMCPeriod;

        this->logger = logger;
        this->boatData = boatData;
        registerConverters();
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


N2kDataToNMEA0183* N2kDataToNMEA0183::create(GwLog *logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, 
    tSendNMEA0183MessageCallback callback, int sourceId,String talkerId){
  LOG_DEBUG(GwLog::LOG,"creating N2kToNMEA0183");    
  return new N2kToNMEA0183Functions(logger,boatData,NMEA2000,callback, sourceId,talkerId);
}
//*****************************************************************************
