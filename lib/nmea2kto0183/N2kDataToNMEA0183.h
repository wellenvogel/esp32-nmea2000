/*
N2kDataToNMEA0183.h

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

#include <NMEA0183.h>
#include <NMEA2000.h>


#include <GwLog.h>
#include <GwBoatData.h>

//------------------------------------------------------------------------------
class N2kDataToNMEA0183 : public tNMEA2000::tMsgHandler {
public:
  using tSendNMEA0183MessageCallback=void (*)(const tNMEA0183Msg &NMEA0183Msg);

    
protected:
  static const unsigned long RMCPeriod=500;
  GwBoatItem<double> *latitude;
  GwBoatItem<double> *longitude;
  GwBoatItem<double> * altitude;
  double Variation;
  GwBoatItem<double> *heading;
  double COG;
  double SOG;
  double STW;
  
  double TWS;
  double TWA; 
  double TWD;
  
  double AWS;
  double AWA;
  double AWD;

  double MaxAws;
  double MaxTws;

  double RudderPosition;
  double WaterTemperature;
  double WaterDepth;
    
  uint32_t TripLog;
  uint32_t Log;

  uint16_t DaysSince1970;
  double SecondsSinceMidnight;
    
  unsigned long LastHeadingTime;
  unsigned long LastCOGSOGTime;
  unsigned long LastPosSend;
  unsigned long LastWindTime;
  unsigned long NextRMCSend;
  unsigned long lastLoopTime;

  GwLog *logger;
  GwBoatData *boatData;

  tNMEA0183 *pNMEA0183;
  tSendNMEA0183MessageCallback SendNMEA0183MessageCallback;

protected:
  void HandleHeading(const tN2kMsg &N2kMsg); // 127250
  void HandleVariation(const tN2kMsg &N2kMsg); // 127258
  void HandleBoatSpeed(const tN2kMsg &N2kMsg); // 128259
  void HandleDepth(const tN2kMsg &N2kMsg); // 128267
  void HandlePosition(const tN2kMsg &N2kMsg); // 129025
  void HandleCOGSOG(const tN2kMsg &N2kMsg); // 129026
  void HandleGNSS(const tN2kMsg &N2kMsg); // 129029
  void HandleWind(const tN2kMsg &N2kMsg); // 130306
  void HandleLog(const tN2kMsg &N2kMsg); // 128275
  void HandleRudder(const tN2kMsg &N2kMsg); // 127245
  void HandleWaterTemp(const tN2kMsg &N2kMsg); // 130310

  
  void SetNextRMCSend() { NextRMCSend=millis()+RMCPeriod; }
  void SendRMC();
  void SendMessage(const tNMEA0183Msg &NMEA0183Msg);

public:
  N2kDataToNMEA0183(GwLog * logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183) ;
  void HandleMsg(const tN2kMsg &N2kMsg);
  void SetSendNMEA0183MessageCallback(tSendNMEA0183MessageCallback _SendNMEA0183MessageCallback) {
    SendNMEA0183MessageCallback=_SendNMEA0183MessageCallback;
  }
  void loop();
};
