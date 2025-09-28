/*
NMEA0183AISMsg.cpp

Copyright (c) 2019 Ronnie Zeiller, www.zeiller.eu
Based on the works of Timo Lappalainen NMEA2000 and NMEA0183 Library

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

#include "NMEA0183AISMsg.h"
#include <NMEA0183Msg.h>
//#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstdio>
#include <sstream>

const char AsciiChar[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&\'()*+,-./0123456789:;<=>?";
const char *tNMEA0183AISMsg::EmptyAISField = "000000";

//*****************************************************************************
tNMEA0183AISMsg::tNMEA0183AISMsg() {
  ClearAIS();
}

//*****************************************************************************
void tNMEA0183AISMsg::ClearAIS() {

  Payload[0]=0;
  PayloadBin.reset();
  iAddPldBin=0;
  iAddPld=0;
}


//*****************************************************************************
bool tNMEA0183AISMsg::AddIntToPayloadBin(int32_t ival, uint16_t countBits) {

  if ( (iAddPldBin + countBits ) >= AIS_BIN_MAX_LEN ) return false; // Is there room for any data

  bset = ival;

  uint16_t iAdd=iAddPldBin;

  for(int i = countBits-1; i >= 0 ; i--) {
    PayloadBin[iAdd]=bset [i];
    iAdd++;
  }

  iAddPldBin += countBits;

  return true;
}

//****************************************************************************
bool tNMEA0183AISMsg::AddBoolToPayloadBin(bool &bval) {
  if ( (iAddPldBin + 1 ) >= AIS_BIN_MAX_LEN ) return false;
  PayloadBin[iAddPldBin]=bval;
  iAddPldBin++; 
  return true;
}

// *****************************************************************************
// converts sval into binary 6-bit AScii encoded string and appends it to PayloadBin
// filled up with "@" == "000000" to given bit-size
bool tNMEA0183AISMsg::AddEncodedCharToPayloadBin(char *sval, size_t countBits) {

  if ( (iAddPldBin + countBits ) >= AIS_BIN_MAX_LEN ) return false; // Is there room for any data

  const char * ptr;
  size_t len = strlen(sval);  // e.g.: should be 7 for Callsign
  if ( len * 6 > countBits ) len = countBits / 6;

  for (size_t i = 0; i<len; i++) {

    ptr = strchr(AsciiChar, sval[i]);
    if ( ptr ) {
      int16_t index = ptr - AsciiChar;
      if (index >= 0){
        AddIntToPayloadBin(index, 6);
      }
    } else {
      AddIntToPayloadBin(0, 6);
    }
  }
  // fill up with "@", also covers empty sval
  if ( len * 6 < countBits ) {
    for (size_t i=0;i<(countBits/6-len);i++) {
      AddIntToPayloadBin(0, 6);
    }
  }
  return true;
}

//*****************************************************************************
template <unsigned int S>
int tNMEA0183AISMsg::ConvertBinaryAISPayloadBinToAscii(std::bitset<S> &src,uint16_t maxSize,uint16_t bitSize,uint16_t stoffset) {
  Payload[0]='\0';
  uint16_t slen=maxSize;
  if (stoffset >= slen) return 0;
  slen-=stoffset;
  uint16_t bitLen=bitSize > 0?bitSize:slen;
  uint16_t len= bitLen / 6; 
  if ((len * 6) < bitLen) len+=1;
  uint16_t padBits=0; 
  uint32_t offset;
  std::bitset<6> s;
  uint8_t dec;
  int i;
  for ( i=0; i<len; i++ ) {
    offset = i * 6;
    int k = 5;
    for (uint32_t j=offset; j<offset+6; j++ ) {
      if (j < slen){
        s[k] = src[stoffset+j];
      }
      else{
        s[k] = 0;
        padBits++;
      }
      k--;
    }
    dec = s.to_ulong();

    if (dec < 40 ) dec += 48;
    else dec += 56;
    char c = dec;
    Payload[i] = c;
  }
  Payload[i]=0;

  return padBits;
}

void tNMEA0183AISMsg::SetChannelAndTalker(bool channelA,bool own){
  channel[0]=channelA?'A':'B';
  strcpy(talker,own?"VDO":"VDM");
}

//**********************  BUILD 2-parted AIS Sentences  ************************
bool tNMEA0183AISMsg::InitAis(int max,int number,int sequence){
  if ( !Init(talker,"AI", '!') ) return false;
  if ( !AddUInt32Field(max) ) return false;
  if ( !AddUInt32Field(number) ) return false;
  if (sequence >= 0){
    if ( !AddUInt32Field(sequence) ) return false;
  }
  else{
    if ( !AddEmptyField() ) return false;
  }
  if ( !AddStrField(channel) ) return false;
  return true;
}
bool  tNMEA0183AISMsg::BuildMsg5Part1() {
  if ( iAddPldBin != 424 ) return false;
  InitAis(2,1,5);
  int padBits=0;
  AddStrField( GetPayload(padBits,0,336));
  AddUInt32Field(padBits);
  return true;
}

bool  tNMEA0183AISMsg::BuildMsg5Part2() {
  if ( iAddPldBin != 424 ) return false;
  InitAis(2,2,5);
  int padBits=0;
  AddStrField( GetPayload(padBits,336,88) );
  AddUInt32Field(padBits);
  return true;
}


//*******************************  AIS PAYLOADS  *********************************
// get converted Payload for Message 1, 2, 3 & 18, always Length 168
const char *tNMEA0183AISMsg::GetPayloadFix(int &padBits,uint16_t fixLen){
  uint16_t lenbin = iAddPldBin;
  if ( lenbin != fixLen ) return nullptr;
  return GetPayload(padBits,0,0);
}
const char *tNMEA0183AISMsg::GetPayload(int &padBits,uint16_t offset,uint16_t bitLen) {
  padBits=ConvertBinaryAISPayloadBinToAscii<AIS_BIN_MAX_LEN>(PayloadBin,iAddPldBin, bitLen,offset );
  return Payload;
}

