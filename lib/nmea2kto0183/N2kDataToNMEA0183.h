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

//------------------------------------------------------------------------------
class N2kDataToNMEA0183 : public tNMEA2000::tMsgHandler
{
public:
  using tSendNMEA0183MessageCallback = void (*)(const tNMEA0183Msg &NMEA0183Msg);

protected:
  GwLog *logger;
  GwBoatData *boatData;

  tNMEA0183 *pNMEA0183;
  tSendNMEA0183MessageCallback SendNMEA0183MessageCallback;


  void SendMessage(const tNMEA0183Msg &NMEA0183Msg);

  N2kDataToNMEA0183(GwLog *logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183);

public:
  static N2kDataToNMEA0183* create(GwLog *logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183);
  virtual void HandleMsg(const tN2kMsg &N2kMsg) = 0;
  void SetSendNMEA0183MessageCallback(tSendNMEA0183MessageCallback _SendNMEA0183MessageCallback)
  {
    SendNMEA0183MessageCallback = _SendNMEA0183MessageCallback;
  }
  virtual void loop();
  virtual ~N2kDataToNMEA0183(){}
};
#endif