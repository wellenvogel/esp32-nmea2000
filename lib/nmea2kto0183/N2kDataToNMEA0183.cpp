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

#include "N2kToNMEA0183Functions.h"
#include "N2kDataToNMEA0183.h"




N2kDataToNMEA0183::N2kDataToNMEA0183(GwLog * logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183) : tNMEA2000::tMsgHandler(0,NMEA2000){
    SendNMEA0183MessageCallback=0;
    pNMEA0183=NMEA0183;
    
  }



//*****************************************************************************
void N2kDataToNMEA0183::loop() {
}

//*****************************************************************************
void N2kDataToNMEA0183::SendMessage(const tNMEA0183Msg &NMEA0183Msg) {
  if ( pNMEA0183 != 0 ) pNMEA0183->SendMessage(NMEA0183Msg);
  if ( SendNMEA0183MessageCallback != 0 ) SendNMEA0183MessageCallback(NMEA0183Msg);
}

N2kDataToNMEA0183* N2kDataToNMEA0183::create(GwLog *logger, GwBoatData *boatData, tNMEA2000 *NMEA2000, tNMEA0183 *NMEA0183){
  return new N2kToNMEA0183Functions(logger,boatData,NMEA2000,NMEA0183);
}
//*****************************************************************************
