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

const double radToDeg = 180.0 / M_PI;

static void updateDouble(GwBoatItem<double> *item,double value){
  if (value == N2kDoubleNA) return;
  if (! item) return;
  item->update(value);
}
static double formatCourse(double cv){
  double rt=cv * radToDeg;
  if (rt > 360) rt-=360;
  if (rt <0) rt+=360;
  return rt;
}
static double formatWind(double cv){
  double rt=formatCourse(cv);
  if (rt > 180) rt=180-rt;
  return rt;
}
static double formatKnots(double cv){
  return cv* 3600.0/1852.0;
}

uint32_t mtr2nm(uint32_t m){
  return m/1852;
}

static void setMax(GwBoatItem<double> *maxItem,GwBoatItem<double> *item){
  unsigned long now=millis();
  if (! item->isValid(now)) return;
  if(item->getData() > maxItem->getData() || ! maxItem->isValid(now)){
    maxItem->update(item->getData());
  }
}

N2kDataToNMEA0183::N2kDataToNMEA0183(GwLog * logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183) : tNMEA2000::tMsgHandler(0,NMEA2000){
    SendNMEA0183MessageCallback=0;
    pNMEA0183=NMEA0183;
    LastPosSend=0;
    lastLoopTime=0;
    NextRMCSend=millis()+RMCPeriod;
  
    this->logger=logger;
    this->boatData=boatData;
    heading=boatData->getDoubleItem(F("Heading"),true,2000,&formatCourse);
    latitude=boatData->getDoubleItem(F("Latitude"),true,4000);
    longitude=boatData->getDoubleItem(F("Longitude"),true,4000);
    altitude=boatData->getDoubleItem(F("Altitude"),true,4000);
    cog=boatData->getDoubleItem(F("COG"),true,2000,&formatCourse);
    sog=boatData->getDoubleItem(F("SOG"),true,2000,&formatKnots);
    stw=boatData->getDoubleItem(F("STW"),true,2000,&formatKnots);
    variation=boatData->getDoubleItem(F("Variation"),true,2000,&formatCourse);
    tws=boatData->getDoubleItem(F("TWS"),true,2000,&formatKnots);
    twd=boatData->getDoubleItem(F("TWD"),true,2000,&formatCourse);
    aws=boatData->getDoubleItem(F("AWS"),true,2000,&formatKnots);
    awa=boatData->getDoubleItem(F("AWA"),true,2000,&formatWind);
    awd=boatData->getDoubleItem(F("AWD"),true,2000,&formatCourse);
    maxAws=boatData->getDoubleItem(F("MaxAws"),true,0,&formatKnots);
    maxTws=boatData->getDoubleItem(F("MaxTws"),true,0,&formatKnots);
    rudderPosition=boatData->getDoubleItem(F("RudderPosition"),true,2000,&formatCourse);
    waterTemperature=boatData->getDoubleItem(F("WaterTemperature"),true,2000,&KelvinToC);
    waterDepth=boatData->getDoubleItem(F("WaterDepth"),true,2000);
    log=boatData->getUint32Item(F("Log"),true,0,mtr2nm);
    tripLog=boatData->getUint32Item(F("TripLog"),true,0,mtr2nm);
    daysSince1970=boatData->getUint32Item(F("DaysSince1970"),true,2000);
    secondsSinceMidnight=boatData->getDoubleItem(F("SecondsSinceMidnight"),true,2000);
  }

//*****************************************************************************
void N2kDataToNMEA0183::HandleMsg(const tN2kMsg &N2kMsg) {
  switch (N2kMsg.PGN) {
    case 127250UL: HandleHeading(N2kMsg);
    case 127258UL: HandleVariation(N2kMsg);
    case 128259UL: HandleBoatSpeed(N2kMsg);
    case 128267UL: HandleDepth(N2kMsg);
    case 129025UL: HandlePosition(N2kMsg);
    case 129026UL: HandleCOGSOG(N2kMsg);
    case 129029UL: HandleGNSS(N2kMsg);
    case 130306UL: HandleWind(N2kMsg);
    case 128275UL: HandleLog(N2kMsg);
    case 127245UL: HandleRudder(N2kMsg);
    case 130310UL: HandleWaterTemp(N2kMsg);
  }
}

//*****************************************************************************
void N2kDataToNMEA0183::loop() {
  unsigned long now=millis();
  if ( now < (lastLoopTime + 100)) return;
  lastLoopTime=now;
  SendRMC();
}

//*****************************************************************************
void N2kDataToNMEA0183::SendMessage(const tNMEA0183Msg &NMEA0183Msg) {
  if ( pNMEA0183 != 0 ) pNMEA0183->SendMessage(NMEA0183Msg);
  if ( SendNMEA0183MessageCallback != 0 ) SendNMEA0183MessageCallback(NMEA0183Msg);
}

//*****************************************************************************
void N2kDataToNMEA0183::HandleHeading(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kHeadingReference ref;
  double _Deviation = 0;
  double Variation;
  tNMEA0183Msg NMEA0183Msg;
  double Heading;
  if ( ParseN2kHeading(N2kMsg, SID, Heading, _Deviation, Variation, ref) ) {
    if ( ref == N2khr_magnetic ) {
      updateDouble(variation,Variation); // Update Variation
      if ( !N2kIsNA(Heading) && variation->isValid(millis()) ) Heading -= variation->getData();
    }
    updateDouble(heading,Heading);
    if ( NMEA0183SetHDG(NMEA0183Msg, heading->getDataWithDefault(NMEA0183DoubleNA), _Deviation, variation->getDataWithDefault(NMEA0183DoubleNA)) ) {
      SendMessage(NMEA0183Msg);
    }
  }
}

//*****************************************************************************
void N2kDataToNMEA0183::HandleVariation(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kMagneticVariation Source;
  uint16_t DaysSince1970;
  double Variation;
  ParseN2kMagneticVariation(N2kMsg, SID, Source, DaysSince1970, Variation);
  updateDouble(variation,Variation);
  if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0) daysSince1970->update(DaysSince1970);
}

//*****************************************************************************
void N2kDataToNMEA0183::HandleBoatSpeed(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double WaterReferenced;
  double GroundReferenced;
  tN2kSpeedWaterReferenceType SWRT;

  if ( ParseN2kBoatSpeed(N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT) ) {
    tNMEA0183Msg NMEA0183Msg;
    updateDouble(stw,WaterReferenced);
    unsigned long now=millis();
    double MagneticHeading = ( heading->isValid(now) && variation->isValid(now)) ? heading->getData() + variation->getData() : NMEA0183DoubleNA;
    if ( NMEA0183SetVHW(NMEA0183Msg, heading->getData(), MagneticHeading, WaterReferenced) ) {
      SendMessage(NMEA0183Msg);
    }
  }
}

//*****************************************************************************
void N2kDataToNMEA0183::HandleDepth(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double DepthBelowTransducer;
  double Offset;
  double Range;
  double WaterDepth;
  if ( ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range) ) {
  
    WaterDepth=DepthBelowTransducer+Offset;
    updateDouble(waterDepth,WaterDepth);  
    tNMEA0183Msg NMEA0183Msg;
    if ( NMEA0183SetDPT(NMEA0183Msg, DepthBelowTransducer, Offset) ) {
      SendMessage(NMEA0183Msg);
    }
    if ( NMEA0183SetDBx(NMEA0183Msg, DepthBelowTransducer, Offset) ) {
      SendMessage(NMEA0183Msg);
    }
  }
}

//*****************************************************************************
void N2kDataToNMEA0183::HandlePosition(const tN2kMsg &N2kMsg) {
  double Latitude;
  double Longitude;
  if ( ParseN2kPGN129025(N2kMsg, Latitude, Longitude) ) {
    updateDouble(latitude,Latitude);
    updateDouble(longitude,Longitude);
  }
}

//*****************************************************************************
void N2kDataToNMEA0183::HandleCOGSOG(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kHeadingReference HeadingReference;
  tNMEA0183Msg NMEA0183Msg;
  double COG;
  double SOG;
  if ( ParseN2kCOGSOGRapid(N2kMsg, SID, HeadingReference, COG, SOG) ) {
    updateDouble(cog,COG);
    updateDouble(sog,SOG);
    double MCOG = ( !N2kIsNA(COG) && variation->isValid()) ? COG - variation->getData() : NMEA0183DoubleNA ;
    if ( HeadingReference == N2khr_magnetic ) {
      MCOG = COG;
      if ( variation->isValid() ) {
        COG -= variation->getData();
        updateDouble(cog,COG);
      }
    }
    if ( NMEA0183SetVTG(NMEA0183Msg, COG, MCOG, SOG) ) {
      SendMessage(NMEA0183Msg);
    }
  }
}

//*****************************************************************************
void N2kDataToNMEA0183::HandleGNSS(const tN2kMsg &N2kMsg) {
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
  if ( ParseN2kGNSS(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSStype, GNSSmethod,
                    nSatellites, HDOP, PDOP, GeoidalSeparation,
                    nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection) ) {
    updateDouble(latitude,Latitude);                  
    updateDouble(longitude,Longitude);
    updateDouble(altitude,Altitude);
    updateDouble(secondsSinceMidnight,SecondsSinceMidnight);
    if (DaysSince1970 != N2kUInt16NA && DaysSince1970 !=0) daysSince1970->update(DaysSince1970);
  }
}

//*****************************************************************************
void N2kDataToNMEA0183::HandleWind(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kWindReference WindReference;
  tNMEA0183WindReference NMEA0183Reference = NMEA0183Wind_True;

  double x, y;
  double WindAngle, WindSpeed;
  
  
  if ( ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference) ) {
    tNMEA0183Msg NMEA0183Msg;
    
    if ( WindReference == N2kWind_Apparent ) {
        NMEA0183Reference = NMEA0183Wind_Apparent;
        updateDouble(awa,WindAngle);
        updateDouble(aws,WindSpeed);
        setMax(maxAws,aws);
       }

    if ( NMEA0183SetMWV(NMEA0183Msg, formatCourse(WindAngle), NMEA0183Reference , WindSpeed)) SendMessage(NMEA0183Msg);

    if (WindReference == N2kWind_Apparent && sog->isValid()) { // Lets calculate and send TWS/TWA if SOG is available
      
      x = WindSpeed * cos(WindAngle);
      y = WindSpeed * sin(WindAngle);

      updateDouble(twd,atan2(y, -sog->getData() + x));
      updateDouble(tws,sqrt(( y*y) + ((- sog->getData()+x)*(-sog->getData()+x))));

      setMax(maxTws,tws);
      
      NMEA0183Reference = NMEA0183Wind_True;
      if ( NMEA0183SetMWV(NMEA0183Msg,
        formatCourse(twd->getData()), 
        NMEA0183Reference , 
        tws->getDataWithDefault(NMEA0183DoubleNA))) SendMessage(NMEA0183Msg);
      double magnetic=twd->getData();
      if (variation->isValid()) magnetic-=variation->getData();
      if ( !NMEA0183Msg.Init("MWD", "GP") ) return;
      if ( !NMEA0183Msg.AddDoubleField(formatCourse(twd->getData())) ) return;
      if ( !NMEA0183Msg.AddStrField("T") ) return;
      if ( !NMEA0183Msg.AddDoubleField(formatCourse(magnetic)) ) return;
      if ( !NMEA0183Msg.AddStrField("M") ) return;
      if ( !NMEA0183Msg.AddDoubleField(tws->getData()/0.514444) ) return;
      if ( !NMEA0183Msg.AddStrField("N") ) return;
      if ( !NMEA0183Msg.AddDoubleField(tws->getData()) ) return;
      if ( !NMEA0183Msg.AddStrField("M") ) return;

      SendMessage(NMEA0183Msg);
     
     }
  }
}
  //*****************************************************************************
  void N2kDataToNMEA0183::SendRMC() {
    long now=millis();
    if ( NextRMCSend <= millis() && latitude->isValid(now) ) {
      tNMEA0183Msg NMEA0183Msg;
      if ( NMEA0183SetRMC(NMEA0183Msg,

        secondsSinceMidnight->getDataWithDefault(NMEA0183DoubleNA), 
        latitude->getDataWithDefault(NMEA0183DoubleNA), 
        longitude->getDataWithDefault(NMEA0183DoubleNA), 
        cog->getDataWithDefault(NMEA0183DoubleNA), 
        sog->getDataWithDefault(NMEA0183DoubleNA),
        daysSince1970->getDataWithDefault(NMEA0183UInt32NA), 
        variation->getDataWithDefault(NMEA0183DoubleNA)
        )
        ) {
        SendMessage(NMEA0183Msg);
      }
      SetNextRMCSend();
    }
  }


  //*****************************************************************************
  void N2kDataToNMEA0183::HandleLog(const tN2kMsg & N2kMsg) {
  uint16_t DaysSince1970;
  double SecondsSinceMidnight;
  uint32_t Log,TripLog;
    if ( ParseN2kDistanceLog(N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog) ) {
      if (Log != N2kUInt32NA) log->update(Log);
      if (TripLog != N2kUInt32NA) tripLog->update(TripLog);
      if (DaysSince1970 != N2kUInt16NA && DaysSince1970 != 0) daysSince1970->update(DaysSince1970);
      tNMEA0183Msg NMEA0183Msg;

      if ( !NMEA0183Msg.Init("VLW", "GP") ) return;
      if ( !NMEA0183Msg.AddDoubleField(Log / 1852.0) ) return;
      if ( !NMEA0183Msg.AddStrField("N") ) return;
      if ( !NMEA0183Msg.AddDoubleField(TripLog / 1852.0) ) return;
      if ( !NMEA0183Msg.AddStrField("N") ) return;

      SendMessage(NMEA0183Msg);
    }
  }

  //*****************************************************************************
  void N2kDataToNMEA0183::HandleRudder(const tN2kMsg & N2kMsg) {

    unsigned char Instance;
    tN2kRudderDirectionOrder RudderDirectionOrder;
    double AngleOrder;
    double RudderPosition;

    if ( ParseN2kRudder(N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder) ) {

      updateDouble(rudderPosition,RudderPosition);
      if(Instance!=0) return;

      tNMEA0183Msg NMEA0183Msg;

      if ( !NMEA0183Msg.Init("RSA", "GP") ) return;
      if ( !NMEA0183Msg.AddDoubleField(formatCourse(RudderPosition)) ) return;
      if ( !NMEA0183Msg.AddStrField("A") ) return;
      if ( !NMEA0183Msg.AddDoubleField(0.0) ) return;
      if ( !NMEA0183Msg.AddStrField("A") ) return;

      SendMessage(NMEA0183Msg);
    }
  }

//*****************************************************************************
  void N2kDataToNMEA0183::HandleWaterTemp(const tN2kMsg & N2kMsg) {

    unsigned char SID;
    double OutsideAmbientAirTemperature;
    double AtmosphericPressure;
    double WaterTemperature;
    if ( ParseN2kPGN130310(N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure) ) {

      updateDouble(waterTemperature,WaterTemperature);
      tNMEA0183Msg NMEA0183Msg;

      if ( !NMEA0183Msg.Init("MTW", "GP") ) return;
      if ( !NMEA0183Msg.AddDoubleField(KelvinToC(WaterTemperature))) return;
      if ( !NMEA0183Msg.AddStrField("C") ) return;
      
      SendMessage(NMEA0183Msg);
    }
  }
