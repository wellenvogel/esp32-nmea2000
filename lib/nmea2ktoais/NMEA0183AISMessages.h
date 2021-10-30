/*
NMEA0183AISMessages.h

Copyright (c) 2019 Ronnie Zeiller, www.zeiller.eu

Based on the works of Timo Lappalainen and Eric S. Raymond and Kurt Schwehr https://gpsd.gitlab.io/gpsd/AIVDM.html

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

#ifndef _tNMEA0183AISMessages_H_
#define _tNMEA0183AISMessages_H_

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <N2kTypes.h>
#include <NMEA0183AISMsg.h>
#include <stddef.h>
#include <vector>
#include <string>

#define MAX_SHIP_IN_VECTOR 200
class ship {
public:
    uint32_t _userID;
    std::string _shipName;

    ship(uint32_t UserID, std::string ShipName) :  _userID(UserID), _shipName(ShipName) {}
};


// Types 1, 2 and 3: Position Report Class A or B
bool SetAISClassABMessage1(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageType, uint8_t Repeat,
                          uint32_t UserID, double Latitude, double Longitude, bool Accuracy, bool RAIM, uint8_t Seconds,
                          double COG, double SOG, double Heading, double ROT, uint8_t NavStatus);

//*****************************************************************************
// AIS Class A Static and Voyage Related Data Message Type 5
bool SetAISClassAMessage5(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat,
                          uint32_t UserID, uint32_t IMONumber, char *Callsign, char *Name,
                          uint8_t VesselType, double Length, double Beam, double PosRefStbd,
                          double PosRefBow, uint16_t ETAdate,  double ETAtime, double Draught,
                          char *Destination, tN2kGNSStype GNSStype, uint8_t DTE );

//*****************************************************************************
// AIS position report (class B 129039) -> Standard Class B CS Position Report Message Type 18 Part B
bool SetAISClassBMessage18(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat, uint32_t UserID,
                                  double Latitude, double Longitude, bool Accuracy, bool RAIM,
                                  uint8_t Seconds, double COG, double SOG, double Heading, tN2kAISUnit Unit,
                                  bool Display, bool DSC, bool Band, bool Msg22, bool Mode, bool State);

//*****************************************************************************
// Static Data Report Class B, Message Type 24
// PGN 129809 Handle AIS Class B "CS" Static Data Report, Part A
bool SetAISClassBMessage24PartA(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat, uint32_t UserID, char *Name);

//*****************************************************************************
// Static Data Report Class B, Message Type 24
bool  SetAISClassBMessage24(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat,
                          uint32_t UserID, uint8_t VesselType, char *VendorID, char *Callsign,
                           double Length, double Beam, double PosRefStbd,  double PosRefBow, uint32_t MothershipID );

int numShips();
inline int32_t aRoundToInt(double x) {
  return x >= 0
      ? (int32_t) floor(x + 0.5)
      : (int32_t) ceil(x - 0.5);
}
#endif
