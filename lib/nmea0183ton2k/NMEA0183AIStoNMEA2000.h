/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

//imported from https://github.com/AK-Homberger/NMEA2000-AIS-Gateway/tree/main/MyAISToN2k
#include <Arduino.h>
#include "ais_decoder.h"
#include "default_sentence_parser.h"
#include "NMEA0183DataToN2K.h"
#include "NMEA0183.h"
#include "GwLog.h"

const double pi = 3.1415926535897932384626433832795;
const double knToms = 1852.0 / 3600.0;
const double degToRad = pi / 180.0;
const double nmTom = 1.852 * 1000;

uint16_t DaysSince1970 = 0;

#define boolbit(b) (b?1:0)

class MyAisDecoder : public AIS::AisDecoder
{
  public:
    int sourceId=-1;
  private:
    NMEA0183DataToN2K::N2kSender sender;
    GwLog *logger;
    void send(const tN2kMsg &msg){
      (*sender)(msg,sourceId);
    }
    AIS::DefaultSentenceParser parser;
  public:
    MyAisDecoder(GwLog *logger,NMEA0183DataToN2K::N2kSender sender)
    {
      this->logger=logger;
      this->sender=sender;
    }

  protected:
    double decodeRot(int iRot){
      //see https://gpsd.gitlab.io/gpsd/AIVDM.html#_type_5_static_and_voyage_related_data
      //and https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:supplementary_software:nmea2000
      double rot=N2kDoubleNA;
      if (iRot == 127) rot=10;
      else if (iRot == -127) rot=-10;
      else if (iRot == 0) rot=0;
      else if ( 1<= iRot && iRot <= 126 ) rot= iRot *iRot / 22.401289;
      else if ( iRot >= -126 && iRot <= -1) rot= iRot * iRot / -22.401289 ;
      //rot now in deg/minute
      rot=rot* degToRad / 60.0; //N"K expects rot in radian/s
      return rot;
    }
    double decodeCog(int iCog){
      double cog = N2kDoubleNA;
      if (iCog >= 0 && iCog < 3600){
        cog= iCog/10.0 * degToRad; 
      }
      return cog;
    }
    double decodeHeading(int iHeading){
      double heading=N2kDoubleNA;
      if ( iHeading >=0 && iHeading <=359){
        heading=iHeading *degToRad;
      }
      return heading;
    }
    virtual void onType123(unsigned int _uMsgType, unsigned int _uMmsi, unsigned int _uNavstatus,
                           int _iRot, unsigned int _uSog, bool _bPosAccuracy,
                           long _iPosLon, long _iPosLat, int _iCog, int _iHeading, int _Repeat, bool _Raim,
                           unsigned int _timestamp, unsigned int _maneuver_i) override {

      // Serial.println("123");

      tN2kMsg N2kMsg;

      // PGN129038
      
      N2kMsg.SetPGN(129038L);
      N2kMsg.Priority = 4;
      N2kMsg.AddByte((_Repeat & 0x03) << 6 | (_uMsgType & 0x3f));
      N2kMsg.Add4ByteUInt(_uMmsi);
      N2kMsg.Add4ByteDouble(_iPosLon / 600000.0, 1e-07);
      N2kMsg.Add4ByteDouble(_iPosLat / 600000.0, 1e-07);
      N2kMsg.AddByte((_timestamp & 0x3f) << 2 | (_Raim & 0x01) << 1 | (_bPosAccuracy & 0x01));
      N2kMsg.Add2ByteUDouble(decodeCog(_iCog), 1e-04);
      N2kMsg.Add2ByteUDouble(_uSog * knToms/10.0, 0.01);
      N2kMsg.AddByte(0x00); // Communication State (19 bits)
      N2kMsg.AddByte(0x00);
      N2kMsg.AddByte(0x00); // AIS transceiver information (5 bits)
      N2kMsg.Add2ByteUDouble(decodeHeading(_iHeading), 1e-04);
      N2kMsg.Add2ByteDouble(decodeRot(_iRot), 3.125E-05); // 1e-3/32.0
      N2kMsg.AddByte(0xF0 | (_uNavstatus & 0x0f));
      N2kMsg.AddByte(0xff); // Reserved
      N2kMsg.AddByte(0xff); // SID (NA)

      send(N2kMsg);
    }

    virtual void onType411(unsigned int , unsigned int , unsigned int , unsigned int , unsigned int , unsigned int , unsigned int , unsigned int , bool , int , int ) override {
      //Serial.println("411");
    }

    virtual void onType5(unsigned int _uMsgType, unsigned int _uMmsi, unsigned int _uImo, const std::string &_strCallsign,
                         const std::string &_strName,
                         unsigned int _uType, unsigned int _uToBow, unsigned int _uToStern,
                         unsigned int _uToPort, unsigned int _uToStarboard, unsigned int _uFixType,
                         unsigned int _uEtaMonth, unsigned int _uEtaDay, unsigned int _uEtaHour,
                         unsigned int _uEtaMinute, unsigned int _uDraught,
                         const std::string &_strDestination, unsigned int _ais_version,
                         unsigned int _repeat, bool _dte) override {

      // Serial.println("5");

      // Necessary due to conflict with TimeLib.h (redefinition of tmElements_t)

      time_t t = DaysSince1970 * (24UL * 3600UL);
      tmElements_t tm;

      tNMEA0183Msg::breakTime(t, tm);

      //tNMEA0183Msg::SetYear(tm, 2020);
      tNMEA0183Msg::SetMonth(tm, _uEtaMonth);
      tNMEA0183Msg::SetDay(tm, _uEtaDay);
      tNMEA0183Msg::SetHour(tm, 0);
      tNMEA0183Msg::SetMin(tm, 0);
      tNMEA0183Msg::SetSec(tm, 0);

      uint16_t eta_days = tNMEA0183Msg::makeTime(tm) / (24UL * 3600UL);

      tN2kMsg N2kMsg;

      char CS[8];
      char Name[21];
      char Dest[21];

      strncpy(CS, _strCallsign.c_str(), sizeof(CS) - 1);
      CS[7] = 0;
      for (int i = strlen(CS); i < 7; i++) CS[i] = 32;

      strncpy(Name, _strName.c_str(), sizeof(Name) - 1);
      Name[20] = 0;
      for (int i = strlen(Name); i < 20; i++) Name[i] = 32;

      strncpy(Dest, _strDestination.c_str(), sizeof(Dest) - 1);
      Dest[20] = 0;
      for (int i = strlen(Dest); i < 20; i++) Dest[i] = 32;
      
      // PGN129794
      SetN2kAISClassAStatic(N2kMsg, _uMsgType, (tN2kAISRepeat) _repeat, _uMmsi,
                            _uImo, CS, Name, _uType, _uToBow + _uToStern,
                            _uToPort + _uToStarboard, _uToStarboard, _uToBow, eta_days,
                            (_uEtaHour * 3600) + (_uEtaMinute * 60), _uDraught / 10.0, Dest,
                            (tN2kAISVersion) _ais_version, (tN2kGNSStype) _uFixType,
                            (tN2kAISDTE) _dte, (tN2kAISTransceiverInformation) _ais_version);

      send(N2kMsg);
    }

    virtual void onType9(unsigned int , unsigned int , bool , int , int , int , unsigned int ) override {
      //Serial.println("9");
    }

    virtual void onType14(unsigned int _repeat, unsigned int _uMmsi,
                          const std::string &_strText, int _iPayloadSizeBits) override {

      tN2kMsg N2kMsg;
      char Text[162];
      strncpy(Text, _strText.c_str(), sizeof(Text)-1);
      Text[161]=0;

      N2kMsg.SetPGN(129802UL);
      N2kMsg.Priority = 4;
      N2kMsg.Destination = 255;  // Redundant, PGN129802 is broadcast by default.
      N2kMsg.AddByte((_repeat & 0x03) << 6 | (14 & 0x3f));
      N2kMsg.Add4ByteUInt(_uMmsi);
      N2kMsg.AddByte(0);

      if (strlen(Text) == 0) {
        N2kMsg.AddByte(0x03); N2kMsg.AddByte(0x01); N2kMsg.AddByte(0x00);
      } else {
        N2kMsg.AddByte(strlen(Text) + 2); N2kMsg.AddByte(0x01);
        for (int i = 0; i < strlen(Text); i++)
          N2kMsg.AddByte(Text[i]);
      }

      send(N2kMsg);
    }

    virtual void onType18(unsigned int _uMsgType, unsigned int _uMmsi, unsigned int _uSog, bool _bPosAccuracy,
                          long _iPosLon, long _iPosLat, int _iCog, int _iHeading, bool _raim, unsigned int _repeat,
                          bool _unit, bool _diplay, bool _dsc, bool _band, bool _msg22, bool _assigned,
                          unsigned int _timestamp, bool _state ) override {
      //Serial.println("18");

      tN2kMsg N2kMsg;

      // PGN129039
      SetN2kAISClassBPosition(N2kMsg, _uMsgType, (tN2kAISRepeat) _repeat, _uMmsi,
                              _iPosLat / 600000.0, _iPosLon / 600000.0, _bPosAccuracy, _raim,
                              _timestamp, decodeCog(_iCog), _uSog * knToms / 10.0,
                              decodeHeading(_iHeading), (tN2kAISUnit) _unit,
                              _diplay, _dsc, _band, _msg22, (tN2kAISMode) _assigned, _state);

      send(N2kMsg);
    }

    virtual void onType19(unsigned int _uMmsi, unsigned int _uSog, bool _bPosAccuracy, int _iPosLon, int _iPosLat,
                          int _iCog, int _iHeading, const std::string &_strName, unsigned int _uType,
                          unsigned int _uToBow, unsigned int _uToStern, unsigned int _uToPort,
                          unsigned int _uToStarboard, unsigned int _timestamp, unsigned int _fixtype,
                          bool _dte, bool _assigned, unsigned int _repeat, bool _raim) override {

      //Serial.println("19");
      tN2kMsg N2kMsg;

      // PGN129040

      char Name[21];
      strncpy(Name, _strName.c_str(), sizeof(Name)-1);
      Name[20]=0;
      for (int i = strlen(Name); i < 20; i++) Name[i] = 32;

      N2kMsg.SetPGN(129040UL);
      N2kMsg.Priority = 4;
      N2kMsg.AddByte((_repeat & 0x03) << 6 | (19 & 0x3f));
      N2kMsg.Add4ByteUInt(_uMmsi);
      N2kMsg.Add4ByteDouble(_iPosLon / 600000.0, 1e-07);
      N2kMsg.Add4ByteDouble(_iPosLat / 600000.0, 1e-07);
      N2kMsg.AddByte((_timestamp & 0x3f) << 2 | (_raim & 0x01) << 1 | (_bPosAccuracy & 0x01));
      N2kMsg.Add2ByteUDouble(decodeCog(_iCog), 1e-04);
      N2kMsg.Add2ByteUDouble(_uSog * knToms / 10.0, 0.01);
      N2kMsg.AddByte(0xff); // Regional Application
      N2kMsg.AddByte(0xff); // Regional Application
      N2kMsg.AddByte(_uType );
      N2kMsg.Add2ByteUDouble(decodeHeading(_iHeading), 1e-04);
      N2kMsg.AddByte(_fixtype << 4);
      N2kMsg.Add2ByteDouble(_uToBow + _uToStern, 0.1);
      N2kMsg.Add2ByteDouble(_uToPort + _uToStarboard, 0.1);
      N2kMsg.Add2ByteDouble(_uToStarboard, 0.1);
      N2kMsg.Add2ByteDouble(_uToBow, 0.1);
      N2kMsg.AddStr(Name, 20);
      N2kMsg.AddByte((_dte & 0x01) | (_assigned & 0x01) << 1) ;
      N2kMsg.AddByte(0);
      N2kMsg.AddByte(0xff); // Sequence ID (Not Available)
      
      send(N2kMsg);      
    }

    //mmsi, aidType, name + nameExt, posAccuracy, posLon, posLat, toBow, toStern, toPort, toStarboard
    virtual void onType21(unsigned int mmsi , unsigned int aidType , const std::string & name, bool accuracy, int posLon, int posLat, unsigned int toBow, unsigned int toStern, unsigned int toPort, unsigned int toStarboard) override {
      //Serial.println("21");
      //in principle we should use tN2kAISAtoNReportData to directly call the library
      //function for 129041. But this makes the conversion really complex.
      int repeat=0; //TODO: should be part of the parameters
      int seconds=0;
      bool raim=false;
      bool offPosition=false;
      bool assignedMode=false;
      bool virtualAton=false;
      tN2kGNSStype gnssType=tN2kGNSStype::N2kGNSSt_GPS; //canboat considers 0 as undefined...
      tN2kAISTransceiverInformation transceiverInfo=tN2kAISTransceiverInformation::N2kaischannel_A_VDL_reception;
      tN2kMsg N2kMsg;
      N2kMsg.SetPGN(129041);
      N2kMsg.Priority=4;
      N2kMsg.AddByte((repeat & 0x03) << 6 | (21 & 0x3f));
      N2kMsg.Add4ByteUInt(mmsi); //N2kData.UserID
      N2kMsg.Add4ByteDouble(posLon / 600000.0, 1e-07);
      N2kMsg.Add4ByteDouble(posLat / 600000.0, 1e-07);
      N2kMsg.AddByte((seconds & 0x3f)<<2 | boolbit(raim)<<1 | boolbit(accuracy)); 
      N2kMsg.Add2ByteUDouble(toBow+toStern, 0.1);
      N2kMsg.Add2ByteUDouble(toPort+toStarboard, 0.1);
      N2kMsg.Add2ByteUDouble(toStarboard, 0.1);
      N2kMsg.Add2ByteUDouble(toBow, 0.1);
      N2kMsg.AddByte(boolbit(assignedMode) << 7 
                    | boolbit(virtualAton) << 6 
                    | boolbit(offPosition) << 5
                    | (aidType & 0x1f)); 
      N2kMsg.AddByte((gnssType & 0x0F) << 1 | 0xe0);
      N2kMsg.AddByte(N2kUInt8NA); //status
      N2kMsg.AddByte((transceiverInfo & 0x1f) | 0xe0);
      N2kMsg.AddVarStr(name.c_str());
      send(N2kMsg);
    }

    virtual void onType24A(unsigned int _uMsgType, unsigned int _repeat, unsigned int _uMmsi,
                           const std::string &_strName) override {
      //Serial.println("24A");

      tN2kMsg N2kMsg;
      char Name[21];
      strncpy(Name, _strName.c_str(), sizeof(Name) - 1);
      Name[20]=0;
      for (int i = strlen(Name); i < 20; i++) Name[i] = 32;
      

      // PGN129809
      SetN2kAISClassBStaticPartA(N2kMsg, _uMsgType, (tN2kAISRepeat) _repeat, _uMmsi, Name);

      send(N2kMsg);
    }

    virtual void onType24B(unsigned int _uMsgType, unsigned int _repeat, unsigned int _uMmsi,
                           const std::string &_strCallsign, unsigned int _uType,
                           unsigned int _uToBow, unsigned int _uToStern, unsigned int _uToPort,
                           unsigned int _uToStarboard, const std::string &_strVendor) override {

      // Serial.println("24B");


      tN2kMsg N2kMsg;
      char CS[8];
      char Vendor[8];

      strncpy(CS, _strCallsign.c_str(), sizeof(CS) - 1);
      CS[7] = 0;
      for (int i = strlen(CS); i < 7; i++) CS[i] = 32;
      strncpy(Vendor, _strVendor.c_str(), sizeof(Vendor) - 1);
      Vendor[7] = 0;
      for (int i = strlen(Vendor); i < 7; i++) Vendor[i] = 32;

      

      // PGN129810
      SetN2kAISClassBStaticPartB(N2kMsg, _uMsgType, (tN2kAISRepeat)_repeat, _uMmsi,
                                 _uType, Vendor, CS, _uToBow + _uToStern, _uToPort + _uToStarboard,
                                 _uToStarboard, _uToBow, _uMmsi);

      send(N2kMsg);
    }

    virtual void onType27(unsigned int , unsigned int , unsigned int , bool , int , int , int ) override {
      //Serial.println("27");
    }

    virtual void onSentence(const AIS::StringRef &_Stc) override {
      //Serial.printf("Sentence: %s\n", _Stc);
    }

    virtual void onMessage(const AIS::StringRef &, const AIS::StringRef &, const AIS::StringRef &) override {}

    virtual void onNotDecoded(const AIS::StringRef &, int ) override {}

    virtual void onDecodeError(const AIS::StringRef &_strMessage, const std::string &_strError) override {
      std::string msg(_strMessage.data(), _strMessage.size());
      AIS::stripTrailingWhitespace(msg);

      LOG_DEBUG(GwLog::ERROR,"%s [%s]\n", _strError.c_str(), msg.c_str());
    }

    virtual void onParseError(const AIS::StringRef &_strMessage, const std::string &_strError) override {
      std::string msg(_strMessage.data(), _strMessage.size());
      AIS::stripTrailingWhitespace(msg);

      LOG_DEBUG(GwLog::ERROR,"%s [%s]\n", _strError.c_str(), msg.c_str());
    }
  public:
    void handleMessage(const char * msg){
      size_t i=decodeMsg(msg,strlen(msg),0,parser,true);
    }
};
