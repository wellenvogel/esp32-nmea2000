/*
NMEA0183AISMsg.h

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

#ifndef _tNMEA0183AISMsg_H_
#define _tNMEA0183AISMsg_H_

#include <NMEA0183Msg.h>
#include <string.h>
#include <time.h>
#include <bitset>
#include <stdint.h>
#include <math.h>
#include <string>


#ifndef AIS_MSG_MAX_LEN
#define AIS_MSG_MAX_LEN 100  // maximum length of AIS Payload
#endif

#ifndef AIS_BIN_MAX_LEN
#define AIS_BIN_MAX_LEN 500  // maximum length of AIS Binary Payload (before encoding to Ascii)
#endif

#define BITSET_LENGTH 120

typedef std::bitset<BITSET_LENGTH> AISBitSet;
class tNMEA0183AISMsg : public tNMEA0183Msg {

  protected:  // AIS-NMEA
    static const char *EmptyAISField;  // 6bits 0      not used yet.....
    static const char *AsciChar;

    uint16_t iAddPldBin;
    char Payload[AIS_MSG_MAX_LEN];
    uint8_t  iAddPld;

  public:
    char PayloadBin[AIS_BIN_MAX_LEN];
    char PayloadBin2[AIS_BIN_MAX_LEN];
    // Clear message
    void ClearAIS();

  public:
    tNMEA0183AISMsg();
    const char *GetPayload();
    const char *GetPayloadType5_Part1();
    const char *GetPayloadType5_Part2();
    const char *GetPayloadType24_PartA();
    const char *GetPayloadType24_PartB();
    const char *GetPayloadBin() const { return  PayloadBin; }

    const tNMEA0183AISMsg& BuildMsg5Part1(tNMEA0183AISMsg &AISMsg);
    const tNMEA0183AISMsg& BuildMsg5Part2(tNMEA0183AISMsg &AISMsg);
    const tNMEA0183AISMsg& BuildMsg24PartA(tNMEA0183AISMsg &AISMsg);
    const tNMEA0183AISMsg& BuildMsg24PartB(tNMEA0183AISMsg &AISMsg);

    // Generally Used
    bool AddIntToPayloadBin(int32_t ival, uint16_t countBits);
    bool AddBoolToPayloadBin(bool &bval, uint8_t size);
    bool AddEncodedCharToPayloadBin(char *sval, size_t Length);
    bool AddEmptyFieldToPayloadBin(uint8_t iBits);
    bool ConvertBinaryAISPayloadBinToAscii(const char *payloadbin);

  // AIS Helper functions
  protected:
    inline int32_t aRoundToInt(double x) {
      return (x >= 0) ? (int32_t) floor(x + 0.5) : (int32_t) ceil(x - 0.5);
    }
};
#endif
