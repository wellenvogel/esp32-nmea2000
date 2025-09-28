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

class tNMEA0183AISMsg : public tNMEA0183Msg {

  protected:  // AIS-NMEA
    std::bitset<BITSET_LENGTH> bset;
    static const char *EmptyAISField;  // 6bits 0      not used yet.....
    static const char *AsciChar;

    uint16_t iAddPldBin;
    char Payload[AIS_MSG_MAX_LEN];
    uint8_t  iAddPld;
    char talker[4]="VDM";
    char channel[2]="A";
    std::bitset<AIS_BIN_MAX_LEN> PayloadBin;
  public:
    // Clear message
    void ClearAIS();

  public:
    tNMEA0183AISMsg();
    const char *GetPayloadFix(int &padBits,uint16_t fixLen=168);
    const char *GetPayload(int &padBits,uint16_t offset=0,uint16_t bitLen=0);

    bool BuildMsg5Part1();
    bool BuildMsg5Part2();
    bool InitAis(int max=1,int number=1,int sequence=-1);

    // Generally Used
    bool AddIntToPayloadBin(int32_t ival, uint16_t countBits);
    bool AddBoolToPayloadBin(bool &bval);
    bool AddEncodedCharToPayloadBin(char *sval, size_t Length);
    /**
     * @param channelA - if set A, otherwise B
     * @param own - if set VDO, else VDM
     */
    void SetChannelAndTalker(bool channelA,bool own=false);
    /**
     * convert the payload to ascii
     * return the number of padding bits
     * @param bitSize the number of bits to be used, 0 - use all bits
     */
    template  <unsigned int SZ>
    int ConvertBinaryAISPayloadBinToAscii(std::bitset<SZ> &src,uint16_t maxSize, uint16_t bitSize,uint16_t offset=0);

  // AIS Helper functions
  protected:
    inline int32_t aRoundToInt(double x) {
      return (x >= 0) ? (int32_t) floor(x + 0.5) : (int32_t) ceil(x - 0.5);
    }
};
#endif
