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
#ifndef _N2KDATATONMEA0183_H
#define _N2KDATATONMEA0183_H
#include <NMEA0183.h>
#include <NMEA2000.h>

#include <GwLog.h>
#include <GwBoatData.h>
#include <GwXDRMappings.h>

//------------------------------------------------------------------------------
class GwJsonDocument;
class N2kDataToNMEA0183 
{
public:
  using tSendNMEA0183MessageCallback = void (*)(const tNMEA0183Msg &NMEA0183Msg, int id);

protected:
  GwLog *logger;
  GwBoatData *boatData;
  int sourceId;
  char talkerId[3];
  tSendNMEA0183MessageCallback SendNMEA0183MessageCallback;
  void SendMessage(const tNMEA0183Msg &NMEA0183Msg);
  N2kDataToNMEA0183(GwLog *logger, GwBoatData *boatData,  
    tSendNMEA0183MessageCallback callback, int sourceId,String talkerId);

public:
  static N2kDataToNMEA0183* create(GwLog *logger, GwBoatData *boatData,  tSendNMEA0183MessageCallback callback, 
    int sourceId,String talkerId, GwXDRMappings *xdrMappings,int minXdrInterval=100);
  virtual void HandleMsg(const tN2kMsg &N2kMsg) = 0;
  virtual void loop();
  virtual ~N2kDataToNMEA0183(){}
  virtual unsigned long* handledPgns()=0;
  virtual int numPgns()=0;
  virtual void toJson(GwJsonDocument *json)=0;
  virtual String handledKeys()=0;
};
#endif