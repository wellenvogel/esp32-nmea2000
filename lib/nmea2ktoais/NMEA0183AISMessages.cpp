/*
NMEA0183AISMessages.cpp

Copyright (c) 2019 Ronnie Zeiller

Based on the works of Timo Lappalainen NMEA2000 and NMEA0183 Library
Thanks to  Eric S. Raymond (https://gpsd.gitlab.io/gpsd/AIVDM.html)
and Kurt Schwehr for their informations on AIS encoding.

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

#include <NMEA0183AISMessages.h>
#include <N2kTypes.h>
#include <N2kMsg.h>
#include <string.h>
//#include <bitset>
//#include <unordered_map>
#include <sstream>
#include <math.h>
#include <NMEA0183AISMsg.h>

const double pi=3.1415926535897932384626433832795;
const double kmhToms=1000.0/3600.0;
const double knToms=1852.0/3600.0;
const double degToRad=pi/180.0;
const double radToDeg=180.0/pi;
const double msTokmh=3600.0/1000.0;
const double msTokn=3600.0/1852.0;
const double nmTom=1.852*1000;
const double mToFathoms=0.546806649;
const double mToFeet=3.2808398950131;
const double radsToDegMin = 60 * 360.0 / (2 * pi);    // [rad/s -> degree/minute]
const char Prefix='!';

std::vector<ship *> vships;

int numShips(){return vships.size();}
// ************************  Helper for AIS  ***********************************
static bool AddMessageType(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageType);
static bool AddRepeat(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t Repeat);
static bool AddUserID(tNMEA0183AISMsg &NMEA0183AISMsg, uint32_t UserID);
static bool AddIMONumber(tNMEA0183AISMsg &NMEA0183AISMsg, uint32_t &IMONumber);
static bool AddText(tNMEA0183AISMsg &NMEA0183AISMsg, char *FieldVal, uint8_t length);
static bool AddDimensions(tNMEA0183AISMsg &NMEA0183AISMsg, double Length, double Beam, double PosRefStbd, double PosRefBow);
static bool AddNavStatus(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t &NavStatus);
static bool AddROT(tNMEA0183AISMsg &NMEA0183AISMsg, double &rot);
static bool AddSOG (tNMEA0183AISMsg &NMEA0183AISMsg, double &sog);
static bool AddLongitude(tNMEA0183AISMsg &NMEA0183AISMsg, double &Longitude);
static bool AddLatitude(tNMEA0183AISMsg &NMEA0183AISMsg, double &Latitude);
static bool AddHeading (tNMEA0183AISMsg &NMEA0183AISMsg, double &heading);
static bool AddCOG(tNMEA0183AISMsg &NMEA0183AISMsg, double cog);
static bool AddSeconds (tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t &Seconds);
static bool AddEPFDFixType(tNMEA0183AISMsg &NMEA0183AISMsg, tN2kGNSStype &GNSStype);
static bool AddStaticDraught(tNMEA0183AISMsg &NMEA0183AISMsg, double &Draught);
static bool AddETADateTime(tNMEA0183AISMsg &NMEA0183AISMsg, uint16_t &ETAdate, double &ETAtime);

//*****************************************************************************
// Types 1, 2 and 3: Position Report Class A or B  -> https://gpsd.gitlab.io/gpsd/AIVDM.html
// total of 168 bits, occupying one AIVDM sentence
// Example: !AIVDM,1,1,,A,133m@ogP00PD;88MD5MTDww@2D7k,0*46
// Payload: Payload: 133m@ogP00PD;88MD5MTDww@2D7k
// Message type 1 has a payload length of 168 bits.
// because AIS encodes messages using a 6-bits ASCII mechanism and 168 divided by 6 is 28.
//
// Got values from: ParseN2kPGN129038()
bool SetAISClassABMessage1( tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageType, uint8_t Repeat,
                          uint32_t UserID, double Latitude, double Longitude, bool Accuracy, bool RAIM, uint8_t Seconds,
                          double COG, double SOG, double Heading, double ROT, uint8_t NavStatus ) {

  NMEA0183AISMsg.ClearAIS();
  if ( !AddMessageType(NMEA0183AISMsg, MessageType) ) return false;    // 0 - 5    | 6    Message Type -> Constant: 1
  if ( !AddRepeat(NMEA0183AISMsg, Repeat) ) return false;              // 6 - 7    | 2    Repeat Indicator: 0 = default; 3 = do not repeat any more
  if ( !AddUserID(NMEA0183AISMsg, UserID) ) return false;              // 8 - 37   | 30  MMSI
  if ( !AddNavStatus(NMEA0183AISMsg, NavStatus) ) return false;        // 38-41    | 4    Navigational Status  e.g.: "Under way sailing"
  if ( !AddROT(NMEA0183AISMsg, ROT) ) return false;                    // 42-49    | 8    Rate of Turn (ROT)
  if ( !AddSOG(NMEA0183AISMsg, SOG) ) return false;                    // 50-59    | 10   [m/s -> kts]  SOG with one digit  x10, 1023 = N/A
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(Accuracy, 1) ) return false;// 60       | 1    GPS Accuracy 1 oder 0, Default 0
  if ( !AddLongitude(NMEA0183AISMsg, Longitude) ) return false;        // 61-88    | 28  Longitude in Minutes / 10000
  if ( !AddLatitude(NMEA0183AISMsg, Latitude) ) return false;          // 89-115   | 27  Latitude in Minutes / 10000
  if ( !AddCOG(NMEA0183AISMsg, COG) ) return false;                    // 116-127  | 12  Course over ground will be 3600 (0xE10) if that data is not available.
  if ( !AddHeading (NMEA0183AISMsg, Heading) ) return false;           // 128-136  |  9    True Heading (HDG)
  if ( !AddSeconds(NMEA0183AISMsg, Seconds) ) return false;            // 137-142  | 6    Seconds in UTC timestamp)
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 2) ) return false;        // 143-144  | 2    Maneuver Indicator: 0 (default) 1, 2  (not delivered within this PGN)
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 3) ) return false;        // 145-147  | 3   Spare
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(RAIM, 1) ) return false;    // 148-148  | 1   RAIM flag 0 = RAIM not in use (default), 1 = RAIM in use
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 19) ) return false;       // 149-167  | 19  Radio Status  (-> 0 NOT SENT WITH THIS PGN!!!!!)

  if ( !NMEA0183AISMsg.Init("VDM","AI", Prefix) ) return false;
  if ( !NMEA0183AISMsg.AddStrField("1") ) return false;
  if ( !NMEA0183AISMsg.AddStrField("1") ) return false;
  if ( !NMEA0183AISMsg.AddEmptyField() ) return false;
  if ( !NMEA0183AISMsg.AddStrField("A") ) return false;
  if ( !NMEA0183AISMsg.AddStrField( NMEA0183AISMsg.GetPayload() ) ) return false;
  if ( !NMEA0183AISMsg.AddStrField("0") ) return false;    // Message 1,2,3 has always Zero Padding

  return true;
}

// *****************************************************************************
// https://www.navcen.uscg.gov/?pageName=AISMessagesAStatic#
// AIS class A Static and Voyage Related Data
// Values derived from ParseN2kPGN129794();
bool  SetAISClassAMessage5(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat,
                          uint32_t UserID, uint32_t IMONumber, char *Callsign, char *Name,
                          uint8_t VesselType, double Length, double Beam, double PosRefStbd,
                          double PosRefBow, uint16_t ETAdate,  double ETAtime, double Draught,
                          char *Destination, tN2kGNSStype GNSStype, uint8_t DTE ) {

  // AIS Type 5 Message
  NMEA0183AISMsg.ClearAIS();
  if ( !AddMessageType(NMEA0183AISMsg, 5) ) return false;                // 0 - 5     | 6    Message Type -> Constant: 5
  if ( !AddRepeat(NMEA0183AISMsg, Repeat) ) return false;                // 6 - 7     | 2    Repeat Indicator: 0 = default; 3 = do not repeat any more
  if ( !AddUserID(NMEA0183AISMsg, UserID) ) return false;                // 8 - 37    | 30  MMSI
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(1, 2) ) return false;          // 38 - 39   |  2   AIS Version  -> 0 oder 1  NOT DERIVED FROM N2k, Always 1!!!!
  if ( !AddIMONumber(NMEA0183AISMsg, IMONumber) ) return false;          // 40 - 69   | 30   IMO Number unisgned
  if ( !AddText(NMEA0183AISMsg, Callsign, 42) ) return false;            // 70 - 111  | 42   Call Sign  WDE4178      -> 7  6-bit characters -> Ascii lt. Table)
  if ( !AddText(NMEA0183AISMsg, Name, 120) ) return false;               // 112-231   | 120 Vessel Name  POINT FERMIN  -> 20 6-bit characters -> Ascii lt. Table
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(VesselType, 8) ) return false; // 232-239   |   8 Ship Type  0....255 e.g. 31  Towing
  if ( !AddDimensions(NMEA0183AISMsg, Length, Beam, PosRefStbd, PosRefBow) ) return false;  // 240 - 269 | 30 Dimensions
  if ( !AddEPFDFixType(NMEA0183AISMsg, GNSStype) ) return false;         // 270-273   | 4  Position Fix Type, 0 (default)
  if ( !AddETADateTime(NMEA0183AISMsg, ETAdate, ETAtime) ) return false; // 274 -293  | 20 Estimated time of arrival; MMDDHHMM UTC
  if ( !AddStaticDraught(NMEA0183AISMsg, Draught) ) return false;        // 294-301   | 8  Maximum Present Static Draught
  if ( !AddText(NMEA0183AISMsg, Destination, 120) ) return false;        // 302-421   | 120 | 20  Destination 20 6-bit characters
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(DTE, 1) ) return false;        // 422       | 1   | Data terminal equipment (DTE) ready (0 = available, 1 = not available = default)
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 1) ) return false;          // 423       | 1   | spare

  return true;
}

//  ****************************************************************************
// AIS position report (class B 129039) -> Type 18: Standard Class B CS Position Report
//  ParseN2kPGN129039(const tN2kMsg &N2kMsg, uint8_t &MessageID, tN2kAISRepeat &Repeat, uint32_t &UserID,
//                        double &Latitude, double &Longitude, bool &Accuracy, bool &RAIM,
//                        uint8_t &Seconds, double &COG, double &SOG, double &Heading, tN2kAISUnit &Unit,
//                        bool &Display, bool &DSC, bool &Band, bool &Msg22, tN2kAISMode &Mode, bool &State)
//  VDM, VDO (AIS VHF Data-link message 18)
bool SetAISClassBMessage18(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat, uint32_t UserID,
                                  double Latitude, double Longitude, bool Accuracy, bool RAIM,
                                  uint8_t Seconds, double COG, double SOG, double Heading, tN2kAISUnit Unit,
                                  bool Display, bool DSC, bool Band, bool Msg22, bool Mode, bool State) {
  //
  NMEA0183AISMsg.ClearAIS();
  if ( !AddMessageType(NMEA0183AISMsg, MessageID) ) return false;      // 0 - 5    | 6    Message Type -> Constant: 18
  if ( !AddRepeat(NMEA0183AISMsg, Repeat) ) return false;              // 6 - 7    | 2    Repeat Indicator: 0 = default; 3 = do not repeat any more
  if ( !AddUserID(NMEA0183AISMsg, UserID) ) return false;              // 8 - 37   | 30  MMSI
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 8) ) return false;        // 38-45    | 8   Regional Reserved
  if ( !AddSOG(NMEA0183AISMsg, SOG) ) return false;                    // 46-55    | 10   [m/s -> kts]  SOG with one digit  x10, 1023 = N/A
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(Accuracy, 1)) return false; // 56       | 1    GPS Accuracy 1 oder 0, Default 0
  if ( !AddLongitude(NMEA0183AISMsg, Longitude) ) return false;        // 57-84    | 28  Longitude in Minutes / 10000
  if ( !AddLatitude(NMEA0183AISMsg, Latitude) ) return false;          // 85-111   | 27  Latitude in Minutes / 10000
  if ( !AddCOG(NMEA0183AISMsg, COG) ) return false;                    // 112-123  | 12  Course over ground will be 3600 (0xE10) if that data is not available.
  if ( !AddHeading (NMEA0183AISMsg, Heading) ) return false;           // 124-132  |  9    True Heading (HDG)
  if ( !AddSeconds(NMEA0183AISMsg, Seconds) ) return false;            // 133-138  | 6    Seconds in UTC timestamp)
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 2) ) return false;        // 139-140  | 2   Regional Reserved
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(Unit, 1) ) return false;     // 141      | 1   0=Class B SOTDMA unit 1=Class B CS (Carrier Sense) unit
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(Display, 1) ) return false;  // 142      | 1    0=No visual display, 1=Has display, (Probably not reliable).
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(DSC, 1) ) return false;     // 143      | 1    If 1, unit is attached to a VHF voice radio with DSC capability.
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(Band, 1) ) return false;    // 144      | 1   If this flag is 1, the unit can use any part of the marine channel.
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(Msg22, 1) ) return false;   // 145      | 1   If 1, unit can accept a channel assignment via Message Type 22.
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(Mode, 1) ) return false;    // 146      | 1   Assigned-mode flag: 0 = autonomous mode (default), 1 = assigned mode
  if ( !NMEA0183AISMsg.AddBoolToPayloadBin(RAIM, 1) ) return false;    // 147      | 1   as for Message Type 1,2,3
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 20) ) return false;       // 148-167  | 20  Radio Status not in PGN 129039

  if ( !NMEA0183AISMsg.Init("VDM","AI", Prefix) ) return false;
  if ( !NMEA0183AISMsg.AddStrField("1") ) return false;
  if ( !NMEA0183AISMsg.AddStrField("1") ) return false;
  if ( !NMEA0183AISMsg.AddEmptyField() ) return false;
  if ( !NMEA0183AISMsg.AddStrField("B") ) return false;
  if ( !NMEA0183AISMsg.AddStrField( NMEA0183AISMsg.GetPayload() ) ) return false;
  if ( !NMEA0183AISMsg.AddStrField("0") ) return false;    // Message 18, has always Zero Padding

  return true;
}

//  ****************************************************************************
//  Type 24: Static Data Report
//  Equivalent of a Type 5 message for ships using Class B equipment. Also used to associate an MMSI
//  with a name on either class A or class B equipment.
//
//  A "Type 24" may be in part A or part B format; According to the standard, parts A and B are expected
//  to be broadcast in adjacent pairs; in the real world they may (due to quirks in various aggregation methods)
//  be separated by other sentences or even interleaved with different Type 24 pairs; decoders must cope with this.
//  The interpretation of some fields in Type B format changes depending on the range of the Type B MMSI field.
//
//  160 bits for part A, 168 bits for part B.
//  According to the standard, both the A and B parts are supposed to be 168 bits.
//  However, in the wild, A parts are often transmitted with only 160 bits, omitting the spare 7 bits at the end.
//  Implementers should be permissive about this.
//
//  If the Part Number field is 0, the rest of the message is interpreted as a Part A;
//  If it is 1, the rest of the message is interpreted as a Part B; values 2 and 3 are not allowed.
//
//  PGN 129809 AIS Class B "CS" Static Data Report, Part A -> AIS VHF Data-link message 24
//  PGN 129810 AIS Class B "CS" Static Data Report, Part B -> AIS VHF Data-link message 24
//   ParseN2kPGN129809 (const tN2kMsg &N2kMsg, uint8_t &MessageID, tN2kAISRepeat &Repeat, uint32_t &UserID, char *Name)  -> store to vector
//  ParseN2kPGN129810(const tN2kMsg &N2kMsg, uint8_t &MessageID, tN2kAISRepeat &Repeat, uint32_t &UserID,
//                      uint8_t &VesselType, char *Vendor, char *Callsign, double &Length, double &Beam,
//                      double &PosRefStbd, double &PosRefBow, uint32_t &MothershipID);
//
//  Part A: MessageID, Repeat, UserID, ShipName -> store in vector to call on Part B arrivals!!!
//  Part B: MessageID, Repeat, UserID, VesselType (5), Callsign (5), Length & Beam, PosRefBow,.. (5)
bool SetAISClassBMessage24PartA(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat, uint32_t UserID, char *Name) {

  bool found = false;
  for (size_t i = 0; i < vships.size(); i++) {
    if ( vships[i]->_userID == UserID ) {
      found = true;
      break;
    }
  }
  if ( ! found ) {
    std::string nm;
    nm+= Name;
    vships.push_back(new ship(UserID, nm));
  }
  return true;
}

// ***************************************************************************************************************
bool  SetAISClassBMessage24(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageID, uint8_t Repeat,
                          uint32_t UserID, uint8_t VesselType, char *VendorID, char *Callsign,
                          double Length, double Beam, double PosRefStbd,  double PosRefBow, uint32_t MothershipID ) {

  uint8_t PartNr = 0;            // Identifier for the message part number; always 0 for Part A
  char *ShipName = (char*)" ";   // get from vector to look up for sent Messages Part A

  uint8_t i;
  for ( i = 0; i < vships.size(); i++) {
    if ( vships[i]->_userID == UserID ) {
      ShipName = const_cast<char*>( vships[i]->_shipName.c_str() );
    }
  }
  if ( i > MAX_SHIP_IN_VECTOR ) {
    std::vector<ship *>::iterator it=vships.begin();
    delete *it;
    vships.erase(it);
  }

  // AIS Type 24 Message
  NMEA0183AISMsg.ClearAIS();
  // Common for PART A AND Part B Bit 0 - 39 / len 40
  if ( !AddMessageType(NMEA0183AISMsg, 24) ) return false;                // 0 - 5    | 6    Message Type -> Constant: 24
  if ( !AddRepeat(NMEA0183AISMsg, Repeat) ) return false;                 // 6 - 7    | 2    Repeat Indicator: 0 = default; 3 = do not repeat any more
  if ( !AddUserID(NMEA0183AISMsg, UserID) ) return false;                 // 8 - 37   | 30  MMSI
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(PartNr, 2) ) return false;      // 38-39    | 2    Part Number 0-1 ->

  // Part A: 40 + 128 = len 168
  if ( !AddText(NMEA0183AISMsg, ShipName, 120) ) return false;            // 40-159   | 120 Vessel Name  20 6-bit characters -> Ascii Table
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 8) ) return false;           // 160-167  | 8    Spare

  // https://www.navcen.uscg.gov/?pageName=AISMessagesB
  // PART B: 40 + 128 = len 168
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(VesselType, 8) ) return false;     // 168-175 | 40-47  |  8    Ship Type 0....99
  if ( !AddText(NMEA0183AISMsg, VendorID, 42) ) return false;                // 176-217 | 48-89  | 42    Vendor ID + Unit Model Code + Serial Number
  if ( !AddText(NMEA0183AISMsg, Callsign, 42) ) return false;                // 218-259 | 90-131 | 42   Call Sign  WDE4178      -> 7  6-bit characters, as in Msg Type 5
  if ( !AddDimensions(NMEA0183AISMsg, Length, Beam, PosRefStbd, PosRefBow) ) return false;  // 260-289 | 132-161 | 30 Dimensions
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(0, 6) ) return false;              // 290-295 | 162-167 | 6    Spare

  return true;
}

//******************************************************************************
//                 Validations and Unit Transformations
//******************************************************************************

// *****************************************************************************
// 6bit    Message Type -> Constant: 1 or 3, 5, 24 etc.
bool AddMessageType(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t MessageType) {

  if (MessageType < 0 || MessageType > 24  ) MessageType = 1;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(MessageType, 6) ) return false;
  return true;
}

// *****************************************************************************
// 2bit    Repeat Indicator: 0 = default; 3 = do not repeat any more
bool AddRepeat(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t Repeat) {

  if (Repeat < 0 || Repeat > 3) Repeat = 0;
  if ( !NMEA0183AISMsg.AddIntToPayloadBin(Repeat, 2) ) return false;
  return true;
}

// *****************************************************************************
// 30bit UserID = MMSI        (9 decimal digits)
bool AddUserID(tNMEA0183AISMsg &NMEA0183AISMsg, uint32_t UserID) {

  if (UserID < 0||UserID > 999999999) UserID = 0;
   if ( !NMEA0183AISMsg.AddIntToPayloadBin(UserID, 30) ) return false;
  return true;
}

// *****************************************************************************
//   30 bit   IMO Number
//   0 = not available = default – Not applicable to SAR aircraft
//  0000000001-0000999999 not used
//  0001000000-0009999999 = valid IMO number;
//  0010000000-1073741823 = official flag state number.
bool AddIMONumber(tNMEA0183AISMsg &NMEA0183AISMsg, uint32_t &IMONumber) {
  uint32_t iTemp;
  ( (IMONumber >= 999999 && IMONumber <= 9999999)||(IMONumber >= 10000000 && IMONumber <= 1073741823) )? iTemp = IMONumber : iTemp = 0;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 30) ) return false;
  return true;
}

// *****************************************************************************
// 42bit Callsign alphanumeric value, max 7 six-bit characters
// 120bit Name or Destination
bool AddText(tNMEA0183AISMsg &NMEA0183AISMsg, char *FieldVal, uint8_t length) {
  uint8_t len = length/6;

  if ( strlen(FieldVal) > len ) FieldVal[len] = 0;
  if ( !NMEA0183AISMsg.AddEncodedCharToPayloadBin(FieldVal, length) ) return false;
  return true;
}

//  *****************************************************************************
//  Calculate Dimension A, B, C, D
//  double PosRefBow      240-248   |   9 [m] Dimension to Bow, reference for pos. A
//  Length -  PosRefBow    249-257  |   9 [m] Dimension to Stern, reference for pos. B
//  Beam - PosRefStbd      258-263  |   6 [m] Dimension to Port, reference for pos. C
//  PosRefStbd             264-269  |   6 [m] Dimension to Starboard, reference for pos. D
//  Ship dimensions will be 0 if not available. For the dimensions to bow and stern,
//  the special value 511 indicates 511 meters or greater;
//  for the dimensions to port and starboard, the special value 63 indicates 63 meters or greater.
// 30 Bit
bool AddDimensions(tNMEA0183AISMsg &NMEA0183AISMsg, double Length, double Beam, double PosRefStbd, double PosRefBow) {
  uint16_t _PosRefBow = 0;
  uint16_t _PosRefStern = 0;
  uint16_t _PosRefStbd = 0;
  uint16_t _PosRefPort = 0;

  if (PosRefBow < 0) PosRefBow=0; //could be N2kIsNA
  if ( PosRefBow <= 511.0 ) {
    _PosRefBow = round(PosRefBow);
  } else {
    _PosRefBow = 511;
  }
  if (PosRefStbd < 0 ) PosRefStbd=0; //could be N2kIsNA
  if (PosRefStbd <= 63.0 ) {
    _PosRefStbd = round(PosRefStbd);
  } else {
    _PosRefStbd = 63;
  }

  if ( !N2kIsNA(Length) ) {
    if (Length >= PosRefBow){
      _PosRefStern=round(Length - PosRefBow);
    }
    if ( _PosRefStern > 511 ) _PosRefStern = 511;
  }
  if ( !N2kIsNA(Beam) ) {
    if (Beam >= PosRefStbd){
      _PosRefPort = round( Beam - PosRefStbd);  
    }
    if ( _PosRefPort > 63 ) _PosRefPort = 63;
  }

  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(_PosRefBow, 9) ) return false;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(_PosRefStern, 9) ) return false;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(_PosRefPort, 6) ) return false;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(_PosRefStbd, 6) ) return false;
  return true;
}

// *****************************************************************************
// 4 Bit  Navigational Status  e.g.: "Under way sailing"
// Same values used as in tN2kAISNavStatus, so we can use direct numbers
bool AddNavStatus(tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t &NavStatus) {
  uint8_t iTemp;
  (NavStatus >= 0 && NavStatus <= 15 )? iTemp = NavStatus : iTemp = 15;
   if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 4) ) return false;
  return true;
}

// *****************************************************************************
// 8bit  [rad/s -> degree/minute]  Rate of Turn ROT  128 = N/A
// 0 = not turning
// 1…126 = turning right at up to 708 degrees per minute or higher
//  1…-126 = turning left at up to 708 degrees per minute or higher
//  127 = turning right at more than 5deg/30s (No TI available)
//  -127 = turning left at more than 5deg/30s (No TI available)
//  128 (80 hex) indicates no turn information available (default)
bool AddROT(tNMEA0183AISMsg &NMEA0183AISMsg, double &rot) {
  int8_t iTemp;
  if ( N2kIsNA(rot)) iTemp = 128;
  else {
    rot *= radsToDegMin;
    (rot > -128.0 && rot < 128.0)? iTemp = aRoundToInt(rot) : iTemp = 128;
  }

  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 8) ) return false;
  return true;
}

// *****************************************************************************
// 10 bit [m/s -> kts]  SOG x10, 1023 = N/A
// Speed over ground is in 0.1-knot resolution from 0 to 102 knots.
// Value 1023 indicates speed is not available, value 1022 indicates 102.2 knots or higher.
bool AddSOG (tNMEA0183AISMsg &NMEA0183AISMsg, double &sog) {
  int16_t iTemp;
  if ( sog < 0.0 ) iTemp = 1023;
  else {
    sog *= msTokn;
    if (sog > 102.2) iTemp = 1023;
    else iTemp = aRoundToInt( 10 * sog );
  }

  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 10) ) return false;
  return true;
}

// *****************************************************************************
// 28 bit   @TODO check negative values
// Values up to plus or minus 180 degrees, East = positive, West = negative.
// A value of 181 degrees (0x6791AC0 hex) indicates that longitude is not available and is the default.
// AIS Longitude is given in in 1/10000 min; divide by 600000.0 to obtain degrees.
bool AddLongitude(tNMEA0183AISMsg &NMEA0183AISMsg, double &Longitude) {
  int32_t iTemp;
  (Longitude >= -180.0 && Longitude <= 180.0)? iTemp = (int) (Longitude * 600000) : iTemp = 181 * 600000;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 28) ) return false;
  return true;
}

// *****************************************************************************
//   27 bit
//  Values up to plus or minus 90 degrees, North = positive, South = negative.
//   A value of 91 degrees (0x3412140 hex) indicates latitude is not available and is the default.
bool AddLatitude(tNMEA0183AISMsg &NMEA0183AISMsg, double &Latitude) {
  int32_t iTemp;
  (Latitude >= -90.0 && Latitude <= 90.0)? iTemp = (int) (Latitude * 600000) : iTemp = 91 * 600000;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 27) ) return false;
  return true;
}

//  ****************************************************************************
// 9 bit True Heading (HDG) 0 to 359 degrees, 511 = not available.
bool AddHeading (tNMEA0183AISMsg &NMEA0183AISMsg, double &heading) {
  uint16_t iTemp;
  if ( N2kIsNA(heading) ) iTemp = 511;
  else {
    heading *= radToDeg;
    (heading >= 0.0 && heading <= 359.0 )? iTemp = aRoundToInt( heading ) : iTemp = 511;
  }
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 9) ) return false;
  return true;
}

// *****************************************************************************
// 12bit Relative to true north, to 0.1 degree precision
bool AddCOG(tNMEA0183AISMsg &NMEA0183AISMsg, double cog) {
  int16_t iTemp;
  cog *= radToDeg;
  if ( cog >= 0.0 && cog < 360.0 ) { iTemp = aRoundToInt( cog * 10 ); } else { iTemp = 3600; }
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 12) ) return false;
  return true;
}

// *****************************************************************************
// 6bit   Seconds in UTC timestamp should be 0-59, except for these special values:
// 60 if time stamp is not available (default)
// 61 if positioning system is in manual input mode
// 62 if Electronic Position Fixing System operates in estimated (dead reckoning) mode,
// 63 if the positioning system is inoperative.
bool AddSeconds (tNMEA0183AISMsg &NMEA0183AISMsg, uint8_t &Seconds) {
  uint8_t iTemp;
  (Seconds >= 0 && Seconds <= 63 )? iTemp = Seconds : iTemp = 60;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(iTemp, 6) ) return false;
  return true;
}

//  *****************************************************************************
//  4 bit  Position Fix Type, See "EPFD Fix Types" 0 (default)
bool AddEPFDFixType(tNMEA0183AISMsg &NMEA0183AISMsg, tN2kGNSStype &GNSStype) {
  // Translate tN2kGNSStype to AIS conventions
  // 3 & 4 not defined in AIS -> we take 1 for GPS
  uint8_t fixType = 0;
  switch (GNSStype) {
    case 0: // GPS
    case 3: // GPS+SBAS/WAAS
    case 4: // GPS+SBAS/WAAS+GLONASS
      fixType = 1; break;
    case 1: // GLONASS
      fixType = 2; break;
    case 2: // GPS+GLONASS
      fixType = 3; break;
    case 5: // Chayka
      fixType = 5; break;
    case 6: // integrated
      fixType = 6; break;
    case 7: // surveyed
      fixType = 7; break;
    case 8: // Galileo
      fixType = 8; break;
    default:
      fixType = 0;
  }
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(fixType, 4) ) return false;
  return true;
}

// *****************************************************************************
// 8 bit Maxiumum present static draught
// In 1/10 m, 255 = draught 25.5 m or greater, 0 = not available = default; in accordance with IMO Resolution A.851
bool AddStaticDraught(tNMEA0183AISMsg &NMEA0183AISMsg, double &Draught) {
  uint8_t staticDraught;
  if ( N2kIsNA(Draught) ) staticDraught = 0;
  else if (Draught < 0.0) staticDraught = 0;
  else if (Draught>25.5) staticDraught = 255;
  else staticDraught = (int) ceil( 10.0 * Draught);

  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(staticDraught, 8) ) return false;
  return true;
}

// *****************************************************************************
// 20bit Estimated time of arrival; MMDDHHMM UTC
// 4 Bits 19-16: month; 1-12; 0 = not available = default
// 5 Bits 15-11: day; 1-31; 0 = not available = default
// 5 Bits 10-6: hour; 0-23; 24 = not available = default
// 6 Bits 5-0: minute; 0-59; 60 = not available = default
// N2k Field #7: ETA Time - Seconds since midnight Bits: 32   Units: s
// Type: Time Resolution: 0.0001 Signed: false    e.g. 36000.00
//  N2k Field #8: ETA Date - Days since January 1, 1970 Bits: 16
//  Units: days Type: Date Resolution: 1 Signed: false  e.g. 18184
bool AddETADateTime(tNMEA0183AISMsg &NMEA0183AISMsg, uint16_t &ETAdate, double &ETAtime) {

  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hour = 24;
  uint8_t minute = 60;

  if (!N2kIsNA(ETAdate) && ETAdate > 0 ) {
    tmElements_t tm;
    #ifndef _Time_h
    time_t t=NMEA0183AISMsg.daysToTime_t(ETAdate);
    #else
    time_t t=ETAdate*86400;
    #endif
    NMEA0183AISMsg.breakTime(t, tm);
    month = (uint8_t) NMEA0183AISMsg.GetMonth(tm);
    day = (uint8_t) NMEA0183AISMsg.GetDay(tm);
  }
  if ( !N2kIsNA(ETAtime) && ETAtime >= 0 ) {
    double temp = ETAtime / 3600;
    hour = (int) temp;
    minute = (int) ((temp - hour) * 60);
  } else {
    hour = 24;
    minute = 60;
  }
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(month, 4) ) return false;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(day, 5) ) return false;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(hour, 5) ) return false;
  if ( ! NMEA0183AISMsg.AddIntToPayloadBin(minute, 6) ) return false;
  return true;
}
