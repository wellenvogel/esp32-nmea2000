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
#include <Arduino.h>
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

  PayloadBin[0]=0;
  Payload[0]=0;
  iAddPldBin=0;
  iAddPld=0;
}

//*****************************************************************************
// Add 6bit with no data.
bool tNMEA0183AISMsg::AddEmptyFieldToPayloadBin(uint8_t iBits) {

  if ( (iAddPldBin + iBits * 6) >= AIS_BIN_MAX_LEN ) return false; // Is there room for any data

  for (uint8_t i=0;i<iBits;i++) {
    strncpy(PayloadBin+iAddPldBin, EmptyAISField, 6);
    iAddPldBin+=6;
  }

  return true;
}

//*****************************************************************************
bool tNMEA0183AISMsg::AddIntToPayloadBin(int32_t ival, uint16_t countBits) {

  if ( (iAddPldBin + countBits ) >= AIS_BIN_MAX_LEN ) return false; // Is there room for any data

  bset = ival;

  PayloadBin[iAddPldBin]=0;
  uint16_t iAdd=iAddPldBin;

  char buf[1];
  for(int i = countBits-1; i >= 0 ; i--) {
    sprintf(buf, "%d", (int) bset[i]);
    PayloadBin[iAdd] = buf[0];
    iAdd++;
  }

  iAddPldBin += countBits;
  PayloadBin[iAddPldBin]=0;

  return true;
}

//  ****************************************************************************
bool tNMEA0183AISMsg::AddBoolToPayloadBin(bool &bval, uint8_t size) {
  int8_t iTemp;
  (bval == true)? iTemp = 1 : iTemp = 0;
  if ( ! AddIntToPayloadBin(iTemp, size) ) return false;
  return true;
}

// *****************************************************************************
// converts sval into binary 6-bit AScii encoded string and appends it to PayloadBin
// filled up with "@" == "000000" to given bit-size
bool tNMEA0183AISMsg::AddEncodedCharToPayloadBin(char *sval, size_t countBits) {

  if ( (iAddPldBin + countBits ) >= AIS_BIN_MAX_LEN ) return false; // Is there room for any data

  PayloadBin[iAddPldBin]=0;
  std::bitset<6> bs;
  char * ptr;
  size_t len = strlen(sval);  // e.g.: should be 7 for Callsign
  if ( len * 6 > countBits ) len = countBits / 6;

  for (int i = 0; i<len; i++) {

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

  PayloadBin[iAddPldBin+1]=0;

  // fill up with "@", also covers empty sval
  if ( len * 6 < countBits ) {
    for (int i=0;i<(countBits/6-len);i++) {
      AddIntToPayloadBin(0, 6);
    }
  }
  PayloadBin[iAddPldBin]=0;
  return true;
}

// *****************************************************************************
bool tNMEA0183AISMsg::ConvertBinaryAISPayloadBinToAscii(const char *payloadbin) {
  uint16_t len;

  len = strlen( payloadbin ) / 6;  // 28
  uint32_t offset;
  char s[7];
  uint8_t dec;
  int i;
  for ( i=0; i<len; i++ ) {
    offset = i * 6;
    int k = 0;
    for (int j=offset; j<offset+6; j++ ) {
      s[k] = payloadbin[j];
       k++;
    }
    s[k]=0;
    dec = strtoull (s, NULL, 2);  //binToDec

    if (dec < 40 ) dec += 48;
    else dec += 56;
    char c = dec;
    Payload[i] = c;
  }
  Payload[i]=0;

  return true;
}

//**********************  BUILD 2-parted AIS Sentences  ************************
const tNMEA0183AISMsg&  tNMEA0183AISMsg::BuildMsg5Part1(tNMEA0183AISMsg &AISMsg) {

  Init("VDM", "AI", '!');
  AddStrField("2");
  AddStrField("1");
  AddStrField("5");
  AddStrField("A");
  AddStrField( GetPayloadType5_Part1() );
  AddStrField("0");

  return AISMsg;
}

const tNMEA0183AISMsg&  tNMEA0183AISMsg::BuildMsg5Part2(tNMEA0183AISMsg &AISMsg) {

  Init("VDM", "AI", '!');
  AddStrField("2");
  AddStrField("2");
  AddStrField("5");
  AddStrField("A");
  AddStrField( GetPayloadType5_Part2() );
  AddStrField("2"); // Message 5, Part 2 has always 2 Padding Zeros

  return AISMsg;
}

const tNMEA0183AISMsg&  tNMEA0183AISMsg::BuildMsg24PartA(tNMEA0183AISMsg &AISMsg) {

  Init("VDM", "AI", '!');
  AddStrField("1");
  AddStrField("1");
  AddEmptyField();
  AddStrField("A");
  AddStrField( GetPayloadType24_PartA() );
  AddStrField("0");

  return AISMsg;
}

const tNMEA0183AISMsg& tNMEA0183AISMsg::BuildMsg24PartB(tNMEA0183AISMsg &AISMsg) {

  Init("VDM", "AI", '!');
  AddStrField("1");
  AddStrField("1");
  AddEmptyField();
  AddStrField("A");
  AddStrField( GetPayloadType24_PartB() );
  AddStrField("0");    // Message 24, both parts have always Zero Padding

  return AISMsg;
}

//*******************************  AIS PAYLOADS  *********************************
//******************************************************************************
// get converted Payload for Message 1, 2, 3 & 18, always Length 168
const char *tNMEA0183AISMsg::GetPayload() {

  uint16_t lenbin = strlen( PayloadBin);
  if ( lenbin != 168 ) return nullptr;

  if ( !ConvertBinaryAISPayloadBinToAscii( PayloadBin ) ) return nullptr;
  return Payload;
}

//******************************************************************************
// get converted Part 1 of Payload for Message 5
const char *tNMEA0183AISMsg::GetPayloadType5_Part1() {

  uint16_t lenbin = strlen( PayloadBin);
  if ( lenbin != 424 ) return nullptr;

  char *to = (char*) malloc(337);
  strncpy(to, PayloadBin, 336);    // First Part is always 336 Length
  to[336]=0;

  if ( !ConvertBinaryAISPayloadBinToAscii( to ) ) return nullptr;

  return Payload;
}

//******************************************************************************
// get converted Part 2 of Payload for Message 5
const char *tNMEA0183AISMsg::GetPayloadType5_Part2() {

  uint16_t lenbin = strlen( PayloadBin);
  if ( lenbin != 424 ) return nullptr;

  lenbin = 88;        // Second Part is always 424 - 336 + 2 padding Zeros in Length
  char *to = (char*) malloc(91);
  strncpy(to, PayloadBin + 336, lenbin);
  to[88]='0'; to[89]='0'; to[90]=0;

  if ( !ConvertBinaryAISPayloadBinToAscii( to ) ) return nullptr;
  return Payload;
}

//******************************************************************************
// get converted Part A of Payload for Message 24
// Bit 0.....167, len 168
// In PayloadBin is Part A and Part B chained together with Length 296
const char *tNMEA0183AISMsg::GetPayloadType24_PartA() {
  uint16_t lenbin = strlen( PayloadBin);
  if ( lenbin != 296 ) return nullptr;    // too short for Part A

  char *to = (char*) malloc(169);    // Part A has Length 168
  *to = '\0';
  for (int i=0; i<168; i++){
    to[i] = PayloadBin[i];
  }
  to[168]=0;
  if ( !ConvertBinaryAISPayloadBinToAscii( to ) ) return nullptr;
  return Payload;

}

//******************************************************************************
// get converted Part B of Payload for Message 24
// Bit 0.....38 + bit39='1' (part number) + bit 168........295  296='\0' of total PayloadBin
// binary part B: len 40 + 128 = len 168
const char *tNMEA0183AISMsg::GetPayloadType24_PartB() {
  uint16_t lenbin = strlen( PayloadBin);
  if ( lenbin != 296 ) return nullptr;    // too short for Part B
  char *to = (char*) malloc(169);    // Part B has Length 168
  *to = '\0';
  for (int i=0; i<39; i++){
    to[i] = PayloadBin[i];
  }
  to[39] = 49;  // part number 1
  for (int i=40; i<168; i++) {
    to[i] = PayloadBin[i+128];
  }
  to[168]=0;
  if ( !ConvertBinaryAISPayloadBinToAscii( to ) ) return nullptr;
  return Payload;
}
