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
#include "GwJsonDocument.h"



N2kDataToNMEA0183::N2kDataToNMEA0183(GwLog * logger, GwBoatData *boatData, 
  SendNMEA0183MessageCallback callback, String talkerId) 
    {
    this->sendNMEA0183MessageCallback=callback;
    strncpy(this->talkerId,talkerId.c_str(),2);
    this->talkerId[2]=0; 
  }



//*****************************************************************************
void N2kDataToNMEA0183::loop(unsigned long) {
}

//*****************************************************************************
void N2kDataToNMEA0183::SendMessage(const tNMEA0183Msg &NMEA0183Msg) {
    sendNMEA0183MessageCallback(NMEA0183Msg, sourceId);
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
    GwXDRMappings *xdrMappings;
    ConverterList<N2kToNMEA0183Functions,tN2kMsg> converters;
    std::map<String,unsigned long> lastSendTransducers;
    tNMEA0183Msg xdrMessage;
    bool xdrOpened=false;
    int xdrCount=0;
    unsigned long lastRmcSent=0;

    bool addToXdr(GwXDRFoundMapping::XdrEntry entry){
        auto it=lastSendTransducers.find(entry.transducer);
        unsigned long now=millis();
        if (it != lastSendTransducers.end()){
            if ((it->second + config.minXdrInterval) > now) return false;
        }
        lastSendTransducers[entry.transducer]=now;
        if (! xdrOpened){
            xdrMessage.Init("XDR",talkerId);
            xdrOpened=true;
            xdrCount=0;
        }
        int len=entry.entry.length();
        if (! xdrMessage.AddStrField(entry.entry.c_str())){
            SendMessage(xdrMessage);
            xdrMessage.Init("XDR",talkerId);
            xdrMessage.AddStrField(entry.entry.c_str());
            xdrCount=1;
        }
        else{
            xdrCount++;
        }
        return true;
    }
    bool finalizeXdr(){
        if (! xdrOpened) return false;
        if ( xdrCount < 1){
            xdrOpened=false;
            return false;
        }
        SendMessage(xdrMessage);
        xdrOpened=false;
        return true;
    }

    void setMax(GwBoatItem<double> *maxItem, GwBoatItem<double> *item)
    {
        unsigned long now = millis();
        if (!item->isValid(now))
            return;
        maxItem->updateMax(item->getData(),sourceId);
    }
    bool updateDouble(GwBoatItem<double> *item, double value)
    {
        if (value == N2kDoubleNA)
            return false;
        if (!item)
            return false;
        return item->update(value,sourceId);
    }
    bool updateDouble(GwXDRFoundMapping * mapping, const double &value){
        if (mapping->empty) return false;
        if (value == N2kDoubleNA)
            return false;
        return boatData->update(value,sourceId,mapping);
    }
    bool updateDouble(GwXDRFoundMapping * mapping, const int8_t &value){
        if (mapping->empty) return false;
        if (value == N2kInt8NA)
            return false;
        return boatData->update((double)value,sourceId,mapping);
    }
    
    
    virtual unsigned long *handledPgns()
    {
        LOG_DEBUG(GwLog::LOG,"CONV: # %d handled PGNS", converters.numConverters());
        return converters.handledPgns();
    }
    virtual String handledKeys(){
        return converters.handledKeys();
    }
    virtual void HandleMsg(const tN2kMsg &N2kMsg, int sourceId)
    {
        this->sourceId=sourceId;
        String key=String(N2kMsg.PGN);
        bool rt=converters.handleMessage(key,N2kMsg,this);
        if (! rt){
          LOG_DEBUG(GwLog::DEBUG+1,"no handler for %ld",N2kMsg.PGN);
        }
        else{
            LOG_DEBUG(GwLog::DEBUG+1,"handled %ld",N2kMsg.PGN);
        }
    }
    virtual void toJson(GwJsonDocument *json)
    {
        converters.toJson(String(F("cnv")),json);
    }
    virtual int numPgns()
    {
        return converters.numConverters();
    }

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
            updateDouble(boatData->VAR,Variation);
            updateDouble(boatData->DEV,_Deviation);
            if (N2kIsNA(Variation)){
                //maybe we still have a valid variation
                Variation=boatData->VAR->getDataWithDefault(N2kDoubleNA);
            }
            if (N2kIsNA(Variation)){
                //no variation
                if (ref == N2khr_magnetic){
                    updateDouble(boatData->HDM,Heading);
                    if (NMEA0183SetHDM(NMEA0183Msg,Heading,talkerId)){
                        SendMessage(NMEA0183Msg);
                    }    
                }
                if (ref == N2khr_true){
                    updateDouble(boatData->HDT,Heading);
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
                updateDouble(boatData->HDM,MagneticHeading);
                updateDouble(boatData->HDT,Heading);
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
        updateDouble(boatData->VAR, Variation);
        if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
            boatData->GPSD->update(DaysSince1970,sourceId);
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
            double MagneticHeading = (boatData->HDT->isValid(now) && boatData->VAR->isValid(now)) ? boatData->HDT->getData() + boatData->VAR->getData() : NMEA0183DoubleNA;
            if (NMEA0183SetVHW(NMEA0183Msg, boatData->HDT->getDataWithDefault(NMEA0183DoubleNA), MagneticHeading, WaterReferenced,talkerId))
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
        if (ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range))
        {
            if (updateDouble(boatData->DBT, DepthBelowTransducer))
            {
                tNMEA0183Msg NMEA0183Msg;
                bool offsetValid=true;
                if (N2kIsNA(Offset)) {
                    Offset=NMEA0183DoubleNA;
                    offsetValid=false;
                }
                if (NMEA0183SetDPT(NMEA0183Msg, DepthBelowTransducer, Offset, talkerId))
                {
                    SendMessage(NMEA0183Msg);
                }
                if (offsetValid)
                {
                    double WaterDepth = DepthBelowTransducer + Offset;
                    updateDouble(boatData->DBS, WaterDepth);
                }
                if (NMEA0183SetDBx(NMEA0183Msg, DepthBelowTransducer, Offset, talkerId))
                {
                    SendMessage(NMEA0183Msg);
                }
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
            updateDouble(boatData->LAT, Latitude);
            updateDouble(boatData->LON, Longitude);
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
            double MCOG = (!N2kIsNA(COG) && boatData->VAR->isValid()) ? COG - boatData->VAR->getData() : NMEA0183DoubleNA;
            if (HeadingReference == N2khr_magnetic)
            {
                MCOG = COG;
                if (boatData->VAR->isValid())
                {
                    COG -= boatData->VAR->getData();
                    updateDouble(boatData->COG, COG);
                }
            }
            if (NMEA0183SetVTG(NMEA0183Msg, COG, MCOG, SOG,talkerId))
            {
                SendMessage(NMEA0183Msg);
            }
        }
    }
    void sendGSA(bool autoMode, int fixMode)
    {
        if (boatData->SatInfo->isValid())
        {
            tNMEA0183Msg nmeaMsg;
            nmeaMsg.Init("GSA", talkerId);
            nmeaMsg.AddStrField(autoMode ? "A" : "M");
            if (fixMode < 1)
                fixMode = 1;
            if (fixMode > 3)
                fixMode = 1;
            nmeaMsg.AddUInt32Field(fixMode);
            for (int i = 0; i < 12; i++)
            {
                GwSatInfo *info = boatData->SatInfo->getAt(i);
                if (info)
                    nmeaMsg.AddUInt32Field(info->PRN);
                else
                    nmeaMsg.AddEmptyField();
            }
            nmeaMsg.AddDoubleField(boatData->PDOP->getDataWithDefault(NMEA0183DoubleNA));
            nmeaMsg.AddDoubleField(boatData->HDOP->getDataWithDefault(NMEA0183DoubleNA));
            nmeaMsg.AddDoubleField(boatData->VDOP->getDataWithDefault(NMEA0183DoubleNA));
            SendMessage(nmeaMsg);
        }
    }

    //*****************************************************************************
    void HandleGNSS(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        tN2kGNSStype GNSStype;
        tN2kGNSSmethod GNSSmethod;
        unsigned char nSatellites;
        double HDOP=N2kDoubleNA;
        double PDOP=N2kDoubleNA;
        double GeoidalSeparation=N2kDoubleNA;
        unsigned char nReferenceStations;
        tN2kGNSStype ReferenceStationType;
        uint16_t ReferenceSationID;
        double AgeOfCorrection=N2kDoubleNA;
        double Latitude=N2kDoubleNA;
        double Longitude=N2kDoubleNA;
        double Altitude=N2kDoubleNA;
        uint16_t DaysSince1970;
        double GpsTime;
        if (ParseN2kGNSS(N2kMsg, SID, DaysSince1970, GpsTime, Latitude, Longitude, Altitude, GNSStype, GNSSmethod,
                         nSatellites, HDOP, PDOP, GeoidalSeparation,
                         nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection))
        {
            updateDouble(boatData->LAT, Latitude);
            updateDouble(boatData->LON, Longitude);
            updateDouble(boatData->ALT, Altitude);
            updateDouble(boatData->GPST, GpsTime);
            updateDouble(boatData->HDOP,HDOP);
            updateDouble(boatData->PDOP,PDOP);
            if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
                boatData->GPSD->update(DaysSince1970,sourceId);
            int quality=0;
            if ((int)GNSSmethod <= 5) quality=(int)GNSSmethod;
            tNMEA0183AISMsg nmeaMsg;
            if (NMEA0183SetGGA(nmeaMsg,GpsTime,Latitude,Longitude,
                quality,nSatellites,HDOP,Altitude,GeoidalSeparation,AgeOfCorrection,
                ReferenceSationID,talkerId)){
                SendMessage(nmeaMsg);
            }    
        }
    }
    void HandleDop(const tN2kMsg &msg){
        double HDOP=N2kDoubleNA;
        double VDOP=N2kDoubleNA;
        double TDOP=N2kDoubleNA;
        tN2kGNSSDOPmode DesiredMode; 
        tN2kGNSSDOPmode ActualMode;
        unsigned char SID;
        if (ParseN2kGNSSDOPData(msg,SID, DesiredMode, ActualMode,
                         HDOP, VDOP, TDOP)){
            updateDouble(boatData->HDOP,HDOP);
            updateDouble(boatData->VDOP,VDOP);
            sendGSA(DesiredMode==N2kGNSSdm_Auto,(int)ActualMode);
        }
    }
    void HandleSats(const tN2kMsg &msg){
        unsigned char SID;
        tN2kRangeResidualMode Mode;
        uint8_t NumberOfSVs;
        tSatelliteInfo info;
        if (ParseN2kPGNSatellitesInView(msg,SID,Mode,NumberOfSVs)){
            for (int i=0;i<NumberOfSVs;i++){
                if (ParseN2kPGNSatellitesInView(msg,i,info)){
                    GwSatInfo satInfo;
                    satInfo.PRN=info.PRN;
                    satInfo.Elevation=RadToDeg(info.Elevation);
                    satInfo.Azimut=RadToDeg(info.Azimuth);
                    satInfo.SNR=info.SNR;
                    if (! boatData->SatInfo->update(satInfo,sourceId)) return;
                }
            }
            NumberOfSVs=boatData->SatInfo->getNumSats();
            if (NumberOfSVs > 0){
                LOG_DEBUG(GwLog::DEBUG+1,"send GSV for %d sats",NumberOfSVs);
                tNMEA0183Msg nmeaMsg;
                int numGSV=NumberOfSVs/4;
                if (numGSV*4 < NumberOfSVs) numGSV++;
                if (numGSV > 9) numGSV=9;
                for (int i=0;i<numGSV ;i++){
                    int idx=i*4;
                    GwSatInfo *i0=boatData->SatInfo->getAt(idx);
                    GwSatInfo *i1=boatData->SatInfo->getAt(idx+1);
                    GwSatInfo *i2=boatData->SatInfo->getAt(idx+2);
                    GwSatInfo *i3=boatData->SatInfo->getAt(idx+3);
                    if (NMEA0183SetGSV(nmeaMsg,numGSV,i+1,NumberOfSVs,
                        i0?i0->PRN:NMEA0183UInt32NA,
                        i0?i0->Elevation:NMEA0183UInt32NA,
                        i0?i0->Azimut:NMEA0183UInt32NA,
                        i0?i0->SNR:NMEA0183UInt32NA,
                        i1?i1->PRN:NMEA0183UInt32NA,
                        i1?i1->Elevation:NMEA0183UInt32NA,
                        i1?i1->Azimut:NMEA0183UInt32NA,
                        i1?i1->SNR:NMEA0183UInt32NA,
                        i2?i2->PRN:NMEA0183UInt32NA,
                        i2?i2->Elevation:NMEA0183UInt32NA,
                        i2?i2->Azimut:NMEA0183UInt32NA,
                        i2?i2->SNR:NMEA0183UInt32NA,
                        i3?i3->PRN:NMEA0183UInt32NA,
                        i3?i3->Elevation:NMEA0183UInt32NA,
                        i3?i3->Azimut:NMEA0183UInt32NA,
                        i3?i3->SNR:NMEA0183UInt32NA,
                        talkerId
                    )){
                        SendMessage(nmeaMsg);
                    }
                }
            }
        }
    }

    //*****************************************************************************
    void HandleWind(const tN2kMsg &N2kMsg)
    {
        unsigned char SID;
        tN2kWindReference WindReference;
        double WindAngle=N2kDoubleNA, WindSpeed=N2kDoubleNA;
        tNMEA0183WindReference NMEA0183Reference;
        if (ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference)) {
            tNMEA0183Msg NMEA0183Msg;
            GwConverterConfig::WindMapping mapping=config.findWindMapping(WindReference);
            bool shouldSend = false;

            // MWV sentence contains apparent/true ANGLE and SPEED
            // https://gpsd.gitlab.io/gpsd/NMEA.html#_mwv_wind_speed_and_angle
            // https://docs.vaisala.com/r/M211109EN-L/en-US/GUID-7402DEF8-5E82-446F-B63E-998F49F3D743/GUID-C77934C7-2A72-466E-BC52-CE6B8CC7ACB6
            if (mapping.valid)
            {
                if (mapping.nmea0183Type == GwConverterConfig::WindMapping::AWA_AWS)
                {
                    NMEA0183Reference = NMEA0183Wind_Apparent;
                    updateDouble(boatData->AWA, WindAngle);
                    updateDouble(boatData->AWS, WindSpeed);
                    setMax(boatData->MaxAws, boatData->AWS);
                    shouldSend = true;
                }
                if (mapping.nmea0183Type == GwConverterConfig::WindMapping::TWA_TWS)
                {
                    NMEA0183Reference = NMEA0183Wind_True;
                    updateDouble(boatData->TWA, WindAngle);
                    updateDouble(boatData->TWS, WindSpeed);
                    setMax(boatData->MaxTws, boatData->TWS);
                    shouldSend = true;
                    if (boatData->HDT->isValid())
                    {
                        double twd = WindAngle + boatData->HDT->getData();
                        if (twd > 2 * M_PI)
                        {
                            twd -= 2 * M_PI;
                        }
                        updateDouble(boatData->TWD, twd);
                    }
                }
                if (mapping.nmea0183Type == GwConverterConfig::WindMapping::TWD_TWS)
                {
                    NMEA0183Reference = NMEA0183Wind_True;
                    updateDouble(boatData->TWD, WindAngle);
                    updateDouble(boatData->TWS, WindSpeed);
                    setMax(boatData->MaxTws, boatData->TWS);
                    if (boatData->HDT->isValid())
                    {
                        shouldSend = true;
                        double twa = WindAngle - boatData->HDT->getData();
                        if (twa > 2 * M_PI)
                        {
                            twa -= 2 * M_PI;
                        }
                        updateDouble(boatData->TWA, twa);
                        WindAngle=twa;
                    }
                }

                if (shouldSend && NMEA0183SetMWV(NMEA0183Msg, formatCourse(WindAngle), NMEA0183Reference, WindSpeed, talkerId))
                {
                    SendMessage(NMEA0183Msg);
                }

                if (shouldSend && NMEA0183Reference == NMEA0183Wind_Apparent)
                {
                    double wa = formatCourse(WindAngle);
                    if (!NMEA0183Msg.Init("VWR", talkerId))
                      return;
                    if (!NMEA0183Msg.AddDoubleField(( wa > 180 ) ? 360-wa : wa))
                      return;
                    if (!NMEA0183Msg.AddStrField(( wa >= 0 && wa <= 180) ? 'R' : 'L'))
                      return;
                    if (!NMEA0183Msg.AddDoubleField(formatKnots(WindSpeed)))
                      return;
                    if (!NMEA0183Msg.AddStrField("N"))
                      return;
                    if (!NMEA0183Msg.AddDoubleField(WindSpeed))
                      return;
                    if (!NMEA0183Msg.AddStrField("M"))
                      return;
                    if (!NMEA0183Msg.AddDoubleField(formatKmh(WindSpeed)))
                      return;
                    if (!NMEA0183Msg.AddStrField("K"))
                      return;

                   SendMessage(NMEA0183Msg);
                }
            }

            /* if (WindReference == N2kWind_Apparent && boatData->SOG->isValid())
            { // Lets calculate and send TWS/TWA if SOG is available

                double x = WindSpeed * cos(WindAngle);
                double y = WindSpeed * sin(WindAngle);

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
                if (boatData->VAR->isValid())
                    magnetic -= boatData->VAR->getData();
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
            } */
        }
    }
    //*****************************************************************************
    void SendRMC()
    {
        long now = millis();
        if (boatData->LAT->isValid(now) && boatData->LON->isValid(now))
        {
            lastRmcSent=now;
            tNMEA0183Msg NMEA0183Msg;
            if (NMEA0183SetRMC(NMEA0183Msg,

                               boatData->GPST->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->LAT->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->LON->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->COG->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->SOG->getDataWithDefault(NMEA0183DoubleNA),
                               boatData->GPSD->getDataWithDefault(NMEA0183UInt32NA),
                               boatData->VAR->getDataWithDefault(NMEA0183DoubleNA),
                               talkerId))
            {
                SendMessage(NMEA0183Msg);
            }
        }
    }

    //*****************************************************************************
    void HandleLog(const tN2kMsg &N2kMsg)
    {
        uint16_t DaysSince1970;
        double GpsTime=N2kDoubleNA;
        uint32_t Log, TripLog;
        if (ParseN2kDistanceLog(N2kMsg, DaysSince1970, GpsTime, Log, TripLog))
        {
            if (Log != N2kUInt32NA)
                boatData->Log->update(Log,sourceId);
            if (TripLog != N2kUInt32NA)
                boatData->TripLog->update(TripLog,sourceId);
            if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0)
                boatData->GPSD->update(DaysSince1970,sourceId);
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
        double AngleOrder=N2kDoubleNA;
        double RudderPosition=N2kDoubleNA;

        if (ParseN2kRudder(N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder))
        {
            if (Instance == config.starboardRudderInstance){
                updateDouble(boatData->RPOS, RudderPosition);
            }
            else if (Instance == config.portRudderInstance){
                updateDouble(boatData->PRPOS, RudderPosition);
            }
            else{
                return;
            }

            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("RSA", talkerId))
                return;
            auto rpos=boatData->RPOS;
            if (rpos->isValid()){
                if (!NMEA0183Msg.AddDoubleField(formatWind(rpos->getData())))return;
                if (!NMEA0183Msg.AddStrField("A"))return;
            }
            else{
                if (!NMEA0183Msg.AddDoubleField(0.0))return;
                if (!NMEA0183Msg.AddStrField("V"))return;
            }
            auto prpos=boatData->PRPOS;
            if (prpos->isValid()){
                if (!NMEA0183Msg.AddDoubleField(formatWind(prpos->getData())))return;
                if (!NMEA0183Msg.AddStrField("A"))return;
            }
            else{
                if (!NMEA0183Msg.AddDoubleField(0.0))return;
                if (!NMEA0183Msg.AddStrField("V"))return;
            }
            SendMessage(NMEA0183Msg);
        }
    }

    //helper for converting the AIS transceiver info to talker/channel

    void setTalkerChannel(tNMEA0183AISMsg &msg, tN2kAISTransceiverInformation &transceiver){
        bool channelA=true;
        bool own=false;
        switch (transceiver){
            case tN2kAISTransceiverInformation::N2kaischannel_A_VDL_reception:
                channelA=true;
                own=false;
                break;
            case tN2kAISTransceiverInformation::N2kaischannel_B_VDL_reception:
                channelA=false;
                own=false;
                break;
            case tN2kAISTransceiverInformation::N2kaischannel_A_VDL_transmission:
                channelA=true;
                own=true;
                break;
            case tN2kAISTransceiverInformation::N2kaischannel_B_VDL_transmission:
                channelA=false;
                own=true;
                break;
        }
        msg.SetChannelAndTalker(channelA,own);
    }

    //*****************************************************************************
    // 129038 AIS Class A Position Report (Message 1, 2, 3)
    void HandleAISClassAPosReport(const tN2kMsg &N2kMsg)
    {

        tN2kAISRepeat _Repeat;
        uint32_t _UserID; // MMSI
        double _Latitude =N2kDoubleNA;
        double _Longitude=N2kDoubleNA;
        bool _Accuracy;
        bool _RAIM;
        uint8_t _Seconds;
        double _COG=N2kDoubleNA;
        double _SOG=N2kDoubleNA;
        double _Heading=N2kDoubleNA;
        double _ROT=N2kDoubleNA;
        tN2kAISNavStatus _NavStatus;
        tN2kAISTransceiverInformation _AISTransceiverInformation; 
        uint8_t _SID;

        uint8_t _MessageType = 1;
        tNMEA0183AISMsg NMEA0183AISMsg;

        if (ParseN2kPGN129038(N2kMsg, _MessageType, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy, _RAIM, _Seconds,
                              _COG, _SOG, _Heading, _ROT, _NavStatus,_AISTransceiverInformation,_SID))
        {


            setTalkerChannel(NMEA0183AISMsg,_AISTransceiverInformation);
            if (_MessageType < 1 || _MessageType > 3) _MessageType=1; //only allow type 1...3 for 129038
            if (SetAISClassABMessage1(NMEA0183AISMsg, _MessageType, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy,
                                      _RAIM, _Seconds, _COG, _SOG, _Heading, _ROT, _NavStatus))
            {

                SendMessage(NMEA0183AISMsg);

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
        double _Length=N2kDoubleNA;
        double _Beam=N2kDoubleNA;
        double _PosRefStbd=N2kDoubleNA;
        double _PosRefBow=N2kDoubleNA;
        uint16_t _ETAdate;
        double _ETAtime=N2kDoubleNA;
        double _Draught=N2kDoubleNA;
        char _Destination[21];
        tN2kAISVersion _AISversion;
        tN2kGNSStype _GNSStype;
        tN2kAISTransceiverInformation _AISinfo;
        tN2kAISDTE _DTE;
        uint8_t _SID;

        tNMEA0183AISMsg NMEA0183AISMsg;

        if (ParseN2kPGN129794(N2kMsg, _MessageID, _Repeat, _UserID, _IMONumber, _Callsign, 8, _Name,21, _VesselType,
                              _Length, _Beam, _PosRefStbd, _PosRefBow, _ETAdate, _ETAtime, _Draught, _Destination,21,
                              _AISversion, _GNSStype, _DTE, _AISinfo,_SID))
        {
            setTalkerChannel(NMEA0183AISMsg,_AISinfo);
            if (SetAISClassAMessage5(NMEA0183AISMsg, _MessageID, _Repeat, _UserID, _IMONumber, _Callsign, _Name, _VesselType,
                                     _Length, _Beam, _PosRefStbd, _PosRefBow, _ETAdate, _ETAtime, _Draught, _Destination,
                                     _GNSStype, _DTE,_AISversion))
            {
                if (NMEA0183AISMsg.BuildMsg5Part1()){
                    SendMessage(NMEA0183AISMsg);
                }
                if (NMEA0183AISMsg.BuildMsg5Part2()){
                    SendMessage(NMEA0183AISMsg);
                }

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
        double _Latitude=N2kDoubleNA;
        double _Longitude=N2kDoubleNA;
        bool _Accuracy;
        bool _RAIM;
        uint8_t _Seconds;
        double _COG=N2kDoubleNA;
        double _SOG=N2kDoubleNA;
        double _Heading=N2kDoubleNA;
        tN2kAISUnit _Unit;
        bool _Display, _DSC, _Band, _Msg22, _State;
        tN2kAISMode _Mode;
        tN2kAISTransceiverInformation  _AISTransceiverInformation;
        uint8_t _SID;

        if (ParseN2kPGN129039(N2kMsg, _MessageID, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy, _RAIM,
                              _Seconds, _COG, _SOG, _AISTransceiverInformation, _Heading, _Unit, _Display, _DSC, _Band, _Msg22, _Mode, _State,_SID))
        {

            tNMEA0183AISMsg NMEA0183AISMsg;
            setTalkerChannel(NMEA0183AISMsg,_AISTransceiverInformation);
            if (SetAISClassBMessage18(NMEA0183AISMsg, _MessageID, _Repeat, _UserID, _Latitude, _Longitude, _Accuracy, _RAIM,
                                      _Seconds, _COG, _SOG,  _Heading, _Unit, _Display, _DSC, _Band, _Msg22, _Mode, _State))
            {

                SendMessage(NMEA0183AISMsg);

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
        tN2kAISTransceiverInformation _AISInfo;
        uint8_t _SID;

        if (ParseN2kPGN129809(N2kMsg, _MessageID, _Repeat, _UserID, _Name,21,_AISInfo,_SID))
        {

            tNMEA0183AISMsg NMEA0183AISMsg;
            setTalkerChannel(NMEA0183AISMsg,_AISInfo);
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
        double _Length=N2kDoubleNA;
        double _Beam=N2kDoubleNA;
        double _PosRefStbd=N2kDoubleNA;
        double _PosRefBow=N2kDoubleNA;
        tN2kAISTransceiverInformation _AISInfo;
        uint8_t _SID;

        if (ParseN2kPGN129810(N2kMsg, _MessageID, _Repeat, _UserID, _VesselType, _Vendor,4, _Callsign,8,
                              _Length, _Beam, _PosRefStbd, _PosRefBow, _MothershipID,_AISInfo,_SID))
        {

            tNMEA0183AISMsg NMEA0183AISMsg;
            setTalkerChannel(NMEA0183AISMsg,_AISInfo);
            if (SetAISClassBMessage24(NMEA0183AISMsg, _MessageID, _Repeat, _UserID, _VesselType, _Vendor, _Callsign,
                                      _Length, _Beam, _PosRefStbd, _PosRefBow, _MothershipID))
            {
                if (NMEA0183AISMsg.BuildMsg24PartA()){
                    SendMessage(NMEA0183AISMsg);
                }

                if (NMEA0183AISMsg.BuildMsg24PartB()){
                    SendMessage(NMEA0183AISMsg);
                }

            }
        }
        return;
    }

    //*****************************************************************************
    // PGN 129041 Aton
    void HandleAISMessage21(const tN2kMsg &N2kMsg)
    {
        tN2kAISAtoNReportData data;
        if (ParseN2kPGN129041(N2kMsg,data)){
            tNMEA0183AISMsg nmea0183Msg;
            setTalkerChannel(nmea0183Msg,data.AISTransceiverInformation);
            if (SetAISMessage21(
                nmea0183Msg,
                data.Repeat,
                data.UserID,
                data.Latitude,
                data.Longitude,
                data.Accuracy,
                data.RAIM,
                data.Seconds,
                data.Length,
                data.Beam,
                data.PositionReferenceStarboard,
                data.PositionReferenceTrueNorth,
                data.AtoNType,
                data.OffPositionIndicator,
                data.VirtualAtoNFlag,
                data.AssignedModeFlag,
                data.GNSSType,
                data.AtoNStatus,
                data.AtoNName
            )){
                SendMessage(nmea0183Msg);
            }
        }
    }

    void HandleSystemTime(const tN2kMsg &msg){
        unsigned char sid=-1;
        uint16_t DaysSince1970=N2kUInt16NA;
        double GpsTime=N2kDoubleNA;
        tN2kTimeSource TimeSource;

        if (! ParseN2kSystemTime(msg,sid,DaysSince1970,GpsTime,TimeSource)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
            return;
        }
        updateDouble(boatData->GPST,GpsTime);
        if (DaysSince1970 != N2kUInt16NA) boatData->GPSD->update(DaysSince1970,sourceId);
        if (boatData->GPSD->isValid() && boatData->GPST->isValid()){
            tNMEA0183Msg nmeaMsg;
            nmeaMsg.Init("ZDA",talkerId);
            char utc[7];
            double seconds=boatData->GPST->getData();
            int hours=floor(seconds/3600.0);
            int minutes=floor(seconds/60) - hours *60;
            int sec=floor(seconds)-60*minutes-3600*hours;
            snprintf(utc,7,"%02d%02d%02d",hours,minutes,sec);
            nmeaMsg.AddStrField(utc);
            tmElements_t timeParts;
            tNMEA0183Msg::breakTime(tNMEA0183Msg::daysToTime_t(boatData->GPSD->getData()),timeParts);
            nmeaMsg.AddUInt32Field(tNMEA0183Msg::GetDay(timeParts));
            nmeaMsg.AddUInt32Field(tNMEA0183Msg::GetMonth(timeParts));
            nmeaMsg.AddUInt32Field(tNMEA0183Msg::GetYear(timeParts));
            if (boatData->TZ->isValid()){
                int hours=boatData->TZ->getData()/60;
                int minutes=boatData->TZ->getData() - 60 *hours;
                nmeaMsg.AddDoubleField(hours,1,"%02.0f");
                nmeaMsg.AddDoubleField(minutes,1,"%02.0f");
            }
            else{
                nmeaMsg.AddEmptyField();
                nmeaMsg.AddEmptyField();
            }
            SendMessage(nmeaMsg);
        }
    }

    void HandleTimeOffset(const tN2kMsg &msg){
        uint16_t DaysSince1970 =N2kUInt16NA;
        double GpsTime=N2kDoubleNA;
        int16_t LocalOffset=N2kInt16NA;
        if (!ParseN2kLocalOffset(msg,DaysSince1970,GpsTime,LocalOffset)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
            return;
        }
        updateDouble(boatData->GPST,GpsTime);
        if (DaysSince1970 != N2kUInt16NA) boatData->GPSD->update(DaysSince1970,sourceId);
        if (LocalOffset != N2kInt16NA) boatData->TZ->update(LocalOffset,sourceId);
    }
    void HandleROT(const tN2kMsg &msg){
        unsigned char SID=0;
        double ROT=N2kDoubleNA;
        if (! ParseN2kRateOfTurn(msg,SID,ROT)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
            return;
        }
        if (!updateDouble(boatData->ROT,ROT)) return;
        tNMEA0183Msg nmeamsg;
        if (NMEA0183SetROT(nmeamsg,ROT * ROT_WA_FACTOR,talkerId)){
            SendMessage(nmeamsg);
        }
    }
    void HandleXTE(const tN2kMsg &msg){
        unsigned char SID=0;
        tN2kXTEMode XTEMode;
        bool NavigationTerminated=false;
        double XTE=N2kDoubleNA;
        if (! ParseN2kXTE(msg,SID,XTEMode,NavigationTerminated,XTE)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
            return; 
        }
        if (NavigationTerminated) return;
        if (! updateDouble(boatData->XTE,XTE)) return;
        tNMEA0183Msg nmeamsg;
        if (!nmeamsg.Init("XTE",talkerId)) return;
        const char *mode="A";
        switch(XTEMode){
            case N2kxtem_Differential:
                mode="D";
                break;
            case N2kxtem_Estimated:
                mode="E";
                break;
            case N2kxtem_Simulator:
                mode="S";
                break;
            case N2kxtem_Manual:
                mode="M";
                break;
            default:
                break;    
        }
        if (!nmeamsg.AddStrField("A")) return;
        if (!nmeamsg.AddStrField("A")) return;
        double val=mtr2nm(XTE);
        const char *dir="L";
        if (val < 0) {
            dir="R";
            val=-val;
        }
        if (! nmeamsg.AddDoubleField(val,1.0,"%.4f")) return;
        if (! nmeamsg.AddStrField(dir)) return;
        if (! nmeamsg.AddStrField("N")) return;
        if (! nmeamsg.AddStrField(mode)) return;
        SendMessage(nmeamsg);
    }

    void HandleNavigation(const tN2kMsg &msg){
        unsigned char SID=0;
        double DistanceToWaypoint=N2kDoubleNA;
        tN2kHeadingReference BearingReference;
        bool PerpendicularCrossed=false; 
        bool ArrivalCircleEntered=false; 
        tN2kDistanceCalculationType CalculationType;
        double ETATime=N2kDoubleNA;
        int16_t ETADate=0;
        double BearingOriginToDestinationWaypoint=N2kDoubleNA;
        double BearingPositionToDestinationWaypoint=N2kDoubleNA;
        uint32_t OriginWaypointNumber; 
        uint32_t DestinationWaypointNumber;
        double DestinationLatitude=N2kDoubleNA;
        double DestinationLongitude=N2kDoubleNA;
        double WaypointClosingVelocity=N2kDoubleNA;
        if (! ParseN2kNavigationInfo(msg,SID,DistanceToWaypoint,BearingReference,
                      PerpendicularCrossed, ArrivalCircleEntered,CalculationType,
                      ETATime, ETADate, BearingOriginToDestinationWaypoint, BearingPositionToDestinationWaypoint,
                      OriginWaypointNumber, DestinationWaypointNumber,
                      DestinationLatitude, DestinationLongitude,WaypointClosingVelocity)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
            return;    
        }
        if (CalculationType != N2kdct_GreatCircle){
            LOG_DEBUG(GwLog::DEBUG,"can only handle %d with great circle type",msg.PGN);
            return;
        }
        if (BearingReference != N2khr_true && BearingReference != N2khr_magnetic){
            LOG_DEBUG(GwLog::DEBUG,"invalid bearing ref  %d for %d ",(int)BearingReference, msg.PGN);
            return;
        }
        if (BearingReference == N2khr_magnetic){
            if (! boatData->VAR->isValid()){
                LOG_DEBUG(GwLog::DEBUG,"missing variation to compute true heading for %d ", msg.PGN);
                return;   
            }
            BearingPositionToDestinationWaypoint-=boatData->VAR->getData();

        }
        if (! updateDouble(boatData->DTW,DistanceToWaypoint)) return;
        if (! updateDouble(boatData->BTW,BearingPositionToDestinationWaypoint)) return;
        if (! updateDouble(boatData->WPLat,DestinationLatitude)) return;
        if (! updateDouble(boatData->WPLon,DestinationLongitude)) return;
        tNMEA0183Msg nmeaMsg;
        if (! nmeaMsg.Init("RMB",talkerId)) return;
        if (! nmeaMsg.AddStrField("A")) return;
        const char *dir="L";
        double xte=boatData->XTE->getDataWithDefault(NMEA0183DoubleNA);
        if (xte != NMEA0183DoubleNA){
            if (xte < 0){
                xte=-xte;
                dir="R";
            }
            xte=mtr2nm(xte);
            if (xte > 9.99) xte=9.99;
        }
        if (! nmeaMsg.AddDoubleField(xte,1.0,"%.2f",dir)) return;
        //TODO: handle waypoint names
        if (! nmeaMsg.AddUInt32Field(OriginWaypointNumber)) return;
        if (! nmeaMsg.AddUInt32Field(DestinationWaypointNumber)) return;
        if (! nmeaMsg.AddLatitudeField(DestinationLatitude)) return;
        if (! nmeaMsg.AddLongitudeField(DestinationLongitude)) return;
        double distance=mtr2nm(DistanceToWaypoint);
        if (distance > 999.9) distance=999.9;
        if (! nmeaMsg.AddDoubleField(distance,1,"%.1f")) return;
        if (! nmeaMsg.AddDoubleField(formatCourse(BearingPositionToDestinationWaypoint))) return;
        if (WaypointClosingVelocity != N2kDoubleNA){
            if (!nmeaMsg.AddDoubleField(msToKnots(WaypointClosingVelocity))) return;
        }
        else{
            if (!nmeaMsg.AddEmptyField()) return;
        }
        if (! nmeaMsg.AddStrField(ArrivalCircleEntered?"A":"V")) return;
        SendMessage(nmeaMsg);
    }

    void HandleFluidLevel(const tN2kMsg &N2kMsg)
    {
        unsigned char Instance;
        tN2kFluidType FluidType;
        double Level=N2kDoubleNA;
        double Capacity=N2kDoubleNA;
        if (ParseN2kPGN127505(N2kMsg,Instance,FluidType,Level,Capacity)) {
            GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRFLUID,FluidType,0,Instance);
            if (updateDouble(&mapping,Level)){
                LOG_DEBUG(GwLog::DEBUG+1,"found fluidlevel mapping %s",mapping.definition->toString().c_str());
                addToXdr(mapping.buildXdrEntry(Level));
            }
            mapping=xdrMappings->getMapping(XDRFLUID,FluidType,1,Instance);
            if (updateDouble(&mapping,Capacity)){
                LOG_DEBUG(GwLog::DEBUG+1,"found fluid capacity mapping %s",mapping.definition->toString().c_str());
                addToXdr(mapping.buildXdrEntry(Capacity));
            }
            finalizeXdr();
        }
    }

    void HandleBatteryStatus(const tN2kMsg &N2kMsg)
    {
        unsigned char SID=-1;
        unsigned char BatteryInstance;
        double BatteryVoltage=N2kDoubleNA;
        double BatteryCurrent=N2kDoubleNA;
        double BatteryTemperature=N2kDoubleNA;
        if (ParseN2kPGN127508(N2kMsg,BatteryInstance,BatteryVoltage,BatteryCurrent,BatteryTemperature,SID)) {
            int i=0;
            GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRBAT,0,0,BatteryInstance);
            if (updateDouble(&mapping,BatteryVoltage)){
                LOG_DEBUG(GwLog::DEBUG+1,"found BatteryVoltage mapping %s",mapping.definition->toString().c_str());
                addToXdr(mapping.buildXdrEntry(BatteryVoltage));
                i++;
            }
            mapping=xdrMappings->getMapping(XDRBAT,0,1,BatteryInstance);
            if (updateDouble(&mapping,BatteryCurrent)){
                LOG_DEBUG(GwLog::DEBUG+1,"found BatteryCurrent mapping %s",mapping.definition->toString().c_str());
                addToXdr(mapping.buildXdrEntry(BatteryCurrent));
                i++;
            }
            mapping=xdrMappings->getMapping(XDRBAT,0,2,BatteryInstance);
            if (updateDouble(&mapping,BatteryTemperature)){
                LOG_DEBUG(GwLog::DEBUG+1,"found BatteryTemperature mapping %s",mapping.definition->toString().c_str());
                addToXdr(mapping.buildXdrEntry(BatteryTemperature));
                i++;
            }
            if (i>0) finalizeXdr();
        }
    }

    void Handle130310(const tN2kMsg &N2kMsg)
    {

        unsigned char SID=-1;
        double OutsideAmbientAirTemperature=N2kDoubleNA;
        double AtmosphericPressure=N2kDoubleNA;
        double WaterTemperature=N2kDoubleNA;
        if (ParseN2kPGN130310(N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure))
        {
            updateDouble(boatData->WTemp, WaterTemperature);
            tNMEA0183Msg NMEA0183Msg;

            if (!NMEA0183Msg.Init("MTW", talkerId))
                return;
            if (!NMEA0183Msg.AddDoubleField(KelvinToC(WaterTemperature)))
                return;
            if (!NMEA0183Msg.AddStrField("C"))
                return;

            SendMessage(NMEA0183Msg);
        }
        int i=0;
        GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRTEMP,N2kts_OutsideTemperature,0,0);
        if (updateDouble(&mapping,OutsideAmbientAirTemperature)){
            LOG_DEBUG(GwLog::DEBUG+1,"found temperature mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(OutsideAmbientAirTemperature));
            i++;
        }
        mapping=xdrMappings->getMapping(XDRPRESSURE,N2kps_Atmospheric,0,0);
        if (updateDouble(&mapping,AtmosphericPressure)){
            LOG_DEBUG(GwLog::DEBUG+1,"found pressure mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(AtmosphericPressure));
            i++;
        }
        if (i>0) finalizeXdr();
    }

    void Handle130311(const tN2kMsg &msg){
        unsigned char SID=-1;
        tN2kTempSource TempSource;
        double Temperature=N2kDoubleNA;
        tN2kHumiditySource HumiditySource;
        double Humidity=N2kDoubleNA;
        double AtmosphericPressure=N2kDoubleNA;
        if (!ParseN2kPGN130311(msg,SID,TempSource,Temperature,HumiditySource,Humidity,AtmosphericPressure)) {
            LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
            return;
        }
        int i=0;
        if (TempSource == N2kts_SeaTemperature) {
          updateDouble(boatData->WTemp, Temperature);
          tNMEA0183Msg NMEA0183Msg;

          if (!NMEA0183Msg.Init("MTW", talkerId))
              return;
          if (!NMEA0183Msg.AddDoubleField(KelvinToC(Temperature)))
              return;
          if (!NMEA0183Msg.AddStrField("C"))
              return;

            SendMessage(NMEA0183Msg);
        }

        GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRTEMP,TempSource,0,0);
        if (updateDouble(&mapping,Temperature)){
            LOG_DEBUG(GwLog::DEBUG+1,"found temperature mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(Temperature));
            i++;
        }
        mapping=xdrMappings->getMapping(XDRHUMIDITY,HumiditySource,0,0);
        if (updateDouble(&mapping,Humidity)){
            LOG_DEBUG(GwLog::DEBUG+1,"found humidity mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(Humidity));
            i++;
        }   
        mapping=xdrMappings->getMapping(XDRPRESSURE,N2kps_Atmospheric,0,0);
        if (updateDouble(&mapping,AtmosphericPressure)){
            LOG_DEBUG(GwLog::DEBUG+1,"found pressure mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(AtmosphericPressure));
            i++;
        }
        if (i>0) finalizeXdr();

    }

    void Handle130312(const tN2kMsg &msg){
        unsigned char SID=-1;
        unsigned char TemperatureInstance=0;
        tN2kTempSource TemperatureSource;
        double Temperature=N2kDoubleNA;
        double setTemperature=N2kDoubleNA;
        if (!ParseN2kPGN130312(msg,SID,TemperatureInstance,TemperatureSource,Temperature,setTemperature)){
           LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
           return;
        }
        if (TemperatureSource == N2kts_SeaTemperature && 
            (config.winst312 == TemperatureInstance || config.winst312 == 256)) {
          updateDouble(boatData->WTemp, Temperature);
          tNMEA0183Msg NMEA0183Msg;

          if (!NMEA0183Msg.Init("MTW", talkerId))
              return;
          if (!NMEA0183Msg.AddDoubleField(KelvinToC(Temperature)))
              return;
          if (!NMEA0183Msg.AddStrField("C"))
              return;

            SendMessage(NMEA0183Msg);
        }

        GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRTEMP,(int)TemperatureSource,0,TemperatureInstance);
        if (updateDouble(&mapping,Temperature)){
            LOG_DEBUG(GwLog::DEBUG+1,"found temperature mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(Temperature));
        }
        mapping=xdrMappings->getMapping(XDRTEMP,(int)TemperatureSource,1,TemperatureInstance);
        if (updateDouble(&mapping,setTemperature)){
            LOG_DEBUG(GwLog::DEBUG+1,"found temperature mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(setTemperature));
        }
        finalizeXdr();
    }

    void Handle130313(const tN2kMsg &msg){
        unsigned char SID=-1;
        unsigned char HumidityInstance=0;
        tN2kHumiditySource HumiditySource;
        double ActualHumidity=N2kDoubleNA;
        double SetHumidity=N2kDoubleNA;
        if (!ParseN2kPGN130313(msg,SID,HumidityInstance,HumiditySource,ActualHumidity,SetHumidity)){
           LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
           return;
        }
        GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRHUMIDITY,(int)HumiditySource,0,HumidityInstance);
        if (updateDouble(&mapping,ActualHumidity)){
            LOG_DEBUG(GwLog::DEBUG+1,"found humidity mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(ActualHumidity));
        }
        mapping=xdrMappings->getMapping(XDRHUMIDITY,(int)HumiditySource,1,HumidityInstance);
        if (updateDouble(&mapping,SetHumidity)){
            LOG_DEBUG(GwLog::DEBUG+1,"found humidity mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(SetHumidity));
        }
        finalizeXdr();
    }

    void Handle130314(const tN2kMsg &msg){
        unsigned char SID=-1;
        unsigned char PressureInstance=0;
        tN2kPressureSource PressureSource;
        double ActualPressure=N2kDoubleNA;
        if (! ParseN2kPGN130314(msg,SID, PressureInstance,
                       PressureSource, ActualPressure)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
            return; 
        }
        GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRPRESSURE,(int)PressureSource,0,PressureInstance);
        if (! updateDouble(&mapping,ActualPressure)) return;
        LOG_DEBUG(GwLog::DEBUG+1,"found pressure mapping %s",mapping.definition->toString().c_str());
        addToXdr(mapping.buildXdrEntry(ActualPressure));
        finalizeXdr();
    }
    void Handle127489(const tN2kMsg &msg){
        unsigned char instance=-1;
        double values[8];
        for (int i=0;i<8;i++) values[i]=N2kDoubleNA;
        int8_t ivalues[2];
        if (! ParseN2kPGN127489(msg,instance,
            values[0],values[1],values[2],values[3],values[4],values[5],
            values[6],values[7],ivalues[0],ivalues[1]
            )){
           LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN); 
        }
        for (int i=0;i<8;i++){
            GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRENGINE,0,i,instance);
            if (! updateDouble(&mapping,values[i])) continue; 
            addToXdr(mapping.buildXdrEntry(values[i])); 
        }
        for (int i=0;i< 2;i++){
            GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRENGINE,0,i+8,instance);
            if (! updateDouble(&mapping,ivalues[i])) continue; 
            addToXdr(mapping.buildXdrEntry((double)ivalues[i])); 
        }
        finalizeXdr();
    }
    void Handle127257(const tN2kMsg &msg){
        unsigned char instance=-1;
        double values[3];
        for (int i=0;i<3;i++) values[i]=N2kDoubleNA;
        //yaw,pitch,roll
        if (! ParseN2kPGN127257(msg,instance,
            values[0],values[1],values[2])){
           LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN); 
        }
        for (int i=0;i<3;i++){
            GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRATTITUDE,0,i,instance);
            if (! updateDouble(&mapping,values[i])) continue; 
            addToXdr(mapping.buildXdrEntry(values[i])); 
        }
        finalizeXdr();
    }
    void Handle127488(const tN2kMsg &msg){
        unsigned char instance=-1;
        double speed=N2kDoubleNA,pressure=N2kDoubleNA;
        int8_t tilt;
        if (! ParseN2kPGN127488(msg,instance,
            speed,pressure,tilt)){
           LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN); 
        }
        GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRENGINE,0,10,instance);
        if (updateDouble(&mapping,speed)){
            addToXdr(mapping.buildXdrEntry(speed)); 
        }
        mapping=xdrMappings->getMapping(XDRENGINE,0,11,instance);
        if (updateDouble(&mapping,pressure)){
            addToXdr(mapping.buildXdrEntry(pressure)); 
        }
        mapping=xdrMappings->getMapping(XDRENGINE,0,12,instance);
        if (updateDouble(&mapping,tilt)){
            addToXdr(mapping.buildXdrEntry((double)tilt)); 
        }
        finalizeXdr();
        if (speed == N2kDoubleNA) return;
        tNMEA0183Msg nmeaMsg;
        if (! nmeaMsg.Init("RPM",talkerId)) return;
        if (! nmeaMsg.AddStrField("E")) return;
        if (! nmeaMsg.AddDoubleField(instance)) return;
        if (! nmeaMsg.AddDoubleField(speed)) return;
        if (! nmeaMsg.AddEmptyField()) return;
        if (! nmeaMsg.AddStrField("V")) return;
        SendMessage(nmeaMsg);
    }

    void Handle130316(const tN2kMsg &msg){
        unsigned char SID=-1;
        unsigned char TemperatureInstance=0;
        tN2kTempSource TemperatureSource;
        double Temperature=N2kDoubleNA;
        double setTemperature=N2kDoubleNA;
        if (!ParseN2kPGN130316(msg,SID,TemperatureInstance,TemperatureSource,Temperature,setTemperature)){
           LOG_DEBUG(GwLog::DEBUG,"unable to parse PGN %d",msg.PGN);
           return;
        }
        GwXDRFoundMapping mapping=xdrMappings->getMapping(XDRTEMP,(int)TemperatureSource,0,TemperatureInstance);
        if (updateDouble(&mapping,Temperature)){
            LOG_DEBUG(GwLog::DEBUG+1,"found temperature mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(Temperature));
        }
        mapping=xdrMappings->getMapping(XDRTEMP,(int)TemperatureSource,1,TemperatureInstance);
        if (updateDouble(&mapping,setTemperature)){
            LOG_DEBUG(GwLog::DEBUG+1,"found temperature mapping %s",mapping.definition->toString().c_str());
            addToXdr(mapping.buildXdrEntry(setTemperature));
        }
        finalizeXdr();
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
      converters.registerConverter(126992UL, &N2kToNMEA0183Functions::HandleSystemTime);
      converters.registerConverter(129033UL, &N2kToNMEA0183Functions::HandleTimeOffset);
      converters.registerConverter(129539UL, &N2kToNMEA0183Functions::HandleDop);
      converters.registerConverter(129540UL, &N2kToNMEA0183Functions::HandleSats);
      converters.registerConverter(127251UL, &N2kToNMEA0183Functions::HandleROT);
      converters.registerConverter(127505UL, &N2kToNMEA0183Functions::HandleFluidLevel);
      converters.registerConverter(127508UL, &N2kToNMEA0183Functions::HandleBatteryStatus);
      converters.registerConverter(129283UL, &N2kToNMEA0183Functions::HandleXTE);
      converters.registerConverter(129284UL, &N2kToNMEA0183Functions::HandleNavigation);
      converters.registerConverter(130310UL, &N2kToNMEA0183Functions::Handle130310);
      converters.registerConverter(130311UL, &N2kToNMEA0183Functions::Handle130311);
      converters.registerConverter(130312UL, &N2kToNMEA0183Functions::Handle130312);
      converters.registerConverter(130313UL, &N2kToNMEA0183Functions::Handle130313);
      converters.registerConverter(130314UL, &N2kToNMEA0183Functions::Handle130314);
      converters.registerConverter(127489UL, &N2kToNMEA0183Functions::Handle127489);
      converters.registerConverter(127488UL, &N2kToNMEA0183Functions::Handle127488);
      converters.registerConverter(130316UL, &N2kToNMEA0183Functions::Handle130316);
      converters.registerConverter(127257UL, &N2kToNMEA0183Functions::Handle127257);
#define HANDLE_AIS
#ifdef HANDLE_AIS
      converters.registerConverter(129038UL, &N2kToNMEA0183Functions::HandleAISClassAPosReport);  // AIS Class A Position Report, Message Type 1
      converters.registerConverter(129039UL, &N2kToNMEA0183Functions::HandleAISClassBMessage18);  // AIS Class B Position Report, Message Type 18
      converters.registerConverter(129794UL, &N2kToNMEA0183Functions::HandleAISClassAMessage5);   // AIS Class A Ship Static and Voyage related data, Message Type 5
      converters.registerConverter(129809UL, &N2kToNMEA0183Functions::HandleAISClassBMessage24A); // AIS Class B "CS" Static Data Report, Part A
      converters.registerConverter(129810UL, &N2kToNMEA0183Functions::HandleAISClassBMessage24B); // AIS Class B "CS" Static Data Report, Part B
      converters.registerConverter(129041UL, &N2kToNMEA0183Functions::HandleAISMessage21);        // AIS Aton
#endif
    }

  public:
    N2kToNMEA0183Functions(GwLog *logger, GwBoatData *boatData, 
        SendNMEA0183MessageCallback callback,
        String talkerId, GwXDRMappings *xdrMappings, const GwConverterConfig &cfg) 
    : N2kDataToNMEA0183(logger, boatData, callback,talkerId)
    {
        this->logger = logger;
        this->boatData = boatData;
        this->xdrMappings=xdrMappings;
        this->config=cfg;
        registerConverters();
    }
    virtual void loop(unsigned long lastExtRmc) override
    {
        N2kDataToNMEA0183::loop(lastExtRmc);
        unsigned long now = millis();
        if (config.rmcInterval > 0 && (lastExtRmc + config.rmcCheckTime) <= now && (lastRmcSent + config.rmcInterval) <= now){
            SendRMC();
        }
    }
};


N2kDataToNMEA0183* N2kDataToNMEA0183::create(GwLog *logger, GwBoatData *boatData, 
    SendNMEA0183MessageCallback callback, String talkerId, GwXDRMappings *xdrMappings,
    const GwConverterConfig &cfg){
  LOG_DEBUG(GwLog::LOG,"creating N2kToNMEA0183");    
  return new N2kToNMEA0183Functions(logger,boatData,callback, talkerId,xdrMappings,cfg);
}
//*****************************************************************************
