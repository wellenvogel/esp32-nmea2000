
#include "ais_decoder.h"
#include "strutils.h"
#include <memory>

#include <stdlib.h>


using namespace AIS;


namespace
{
#ifdef WIN32
template <typename int_type>
int_type bswap64(int_type _i) {
  return _byteswap_uint64(_i);
}
#else
template <typename int_type>
int_type bswap64(int_type _i) {
  return __builtin_bswap64(_i);
}
#endif
}


PayloadBuffer::PayloadBuffer()
  : m_data{},
    m_iBitIndex(0)
{}

/* set bit index back to zero */
void PayloadBuffer::resetBitIndex()
{
  m_iBitIndex = 0;
}

/* unpack next _iBits (most significant bit is packed first) */
unsigned int PayloadBuffer::getUnsignedValue(int _iBits)
{
  const unsigned char *lptr = &m_data[m_iBitIndex >> 3];
  uint64_t bits = (uint64_t)lptr[0] << 40;
  bits |= (uint64_t)lptr[1] << 32;

  if (_iBits > 9) {
    bits |= (unsigned int)lptr[2] << 24;
    bits |= (unsigned int)lptr[3] << 16;
    bits |= (unsigned int)lptr[4] << 8;
    bits |= (unsigned int)lptr[5];
  }

  bits <<= 16 + (m_iBitIndex & 7);
  m_iBitIndex += _iBits;

  return (unsigned int)(bits >> (64 - _iBits));
}

/* unpack next _iBits (most significant bit is packed first; with sign check/conversion) */
int PayloadBuffer::getSignedValue(int _iBits)
{
  const unsigned char *lptr = &m_data[m_iBitIndex >> 3];
  uint64_t bits = (uint64_t)lptr[0] << 40;
  bits |= (uint64_t)lptr[1] << 32;

  if (_iBits > 9) {
    bits |= (unsigned int)lptr[2] << 24;
    bits |= (unsigned int)lptr[3] << 16;
    bits |= (unsigned int)lptr[4] << 8;
    bits |= (unsigned int)lptr[5];
  }

  bits <<= 16 + (m_iBitIndex & 7);
  m_iBitIndex += _iBits;

  return (int)((int64_t)bits >> (64 - _iBits));
}

/* unback string (6 bit characters) -- already cleans string (removes trailing '@' and trailing spaces) */
std::string PayloadBuffer::getString(int _iNumBits)
{
  static thread_local std::array<char, 64> strdata;

  int iNumChars = _iNumBits / 6;
  if (iNumChars > (int)strdata.size())
  {
    iNumChars = (int)strdata.size();
  }

  int32_t iStartBitIndex = m_iBitIndex;

  for (int i = 0; i < iNumChars; i++)
  {
    unsigned int ch = getUnsignedValue(6);
    if (ch > 0) // stop on '@'
    {
      strdata[i] = ASCII_CHARS[ch];
    }
    else
    {
      iNumChars = i;
      break;
    }
  }

  // remove trailing spaces
  while (iNumChars > 0)
  {
    if (ascii_isspace(strdata[iNumChars - 1]) == true)
    {
      iNumChars--;
    }
    else
    {
      break;
    }
  }

  // make sure bit index is correct
  m_iBitIndex = iStartBitIndex + _iNumBits;

  return std::string(strdata.data(), iNumChars);
}

/* convert payload to decimal (de-armour) and concatenate 6bit decimal values into payload buffer */
int AIS::decodeAscii(PayloadBuffer &_buffer, const StringRef &_strPayload, int _iFillBits)
{
  static const unsigned char dLUT[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  const unsigned char* in_ptr = (unsigned char*)_strPayload.data();
  const unsigned char* in_sentinel = in_ptr + _strPayload.size();
  const unsigned char* in_sentinel4 = in_sentinel - 4;
  unsigned char* out_ptr = _buffer.getData();

  uint64_t accumulator = 0;
  unsigned int acc_bitcount = 0;

  while (in_ptr < in_sentinel4) {
    uint64_t val = (dLUT[*in_ptr] << 18) | (dLUT[*(in_ptr + 1)] << 12) | (dLUT[*(in_ptr + 2)] << 6) | dLUT[*(in_ptr + 3)];

    constexpr unsigned int nbits = 24;

    int remainder = int(64 - acc_bitcount) - nbits;
    if (remainder <= 0) {
      // accumulator will fill up, commit to output buffer
      accumulator |= (uint64_t(val) >> -remainder);
      *((uint64_t*)out_ptr) = bswap64(accumulator);
      out_ptr += 8;

      if (remainder < 0) {
        accumulator = uint64_t(val) << (64 + remainder); // remainder is negative
        acc_bitcount = -remainder;
      } else {
        accumulator = 0;  // shifting right by 64 bits (above) does not yield zero?
        acc_bitcount = 0;
      }

    } else {
      // we still have enough room in the accumulator
      accumulator |= uint64_t(val) << (64 - acc_bitcount - nbits);
      acc_bitcount += nbits;
    }

    in_ptr += 4;
  }

  while (in_ptr < in_sentinel) {
    uint64_t val = dLUT[*in_ptr];

    constexpr unsigned int nbits = 6;

    int remainder = int(64 - acc_bitcount) - nbits;
    if (remainder <= 0) {
      // accumulator will fill up, commit to output buffer
      accumulator |= (uint64_t(val) >> -remainder);
      *((uint64_t*)out_ptr) = bswap64(accumulator);
      out_ptr += 8;

      if (remainder < 0) {
        accumulator = uint64_t(val) << (64 + remainder); // remainder is negative
        acc_bitcount = -remainder;
      } else {
        accumulator = 0;  // shifting right by 64 bits (above) does not yield zero?
        acc_bitcount = 0;
      }

    } else {
      // we still have enough room in the accumulator
      accumulator |= uint64_t(val) << (64 - acc_bitcount - nbits);
      acc_bitcount += nbits;
    }

    in_ptr++;
  }
  *((uint64_t*)out_ptr) = bswap64(accumulator);

  return (int)(_strPayload.size() * 6 - _iFillBits);
}


/* calc CRC */
uint8_t AIS::crc(const StringRef &_strNmea)
{
  const unsigned char* in_ptr = (const unsigned char*)_strNmea.data();
  const unsigned char* in_sentinel  = in_ptr + _strNmea.size();
  const unsigned char* in_sentinel4 = in_sentinel - 4;

  uint8_t checksum = 0;
  while ((intptr_t(in_ptr) & 3) && in_ptr < in_sentinel) {
    checksum ^= *in_ptr++;
  }

  uint32_t checksum4 = checksum;
  while (in_ptr < in_sentinel4) {
    checksum4 ^= *((uint32_t*)in_ptr);
    in_ptr += 4;
  }

  checksum = (checksum4 & 0xff) ^ ((checksum4 >> 8) & 0xff) ^ ((checksum4 >> 16) & 0xff) ^ ((checksum4 >> 24) & 0xff);

  while (in_ptr < in_sentinel) {
    checksum ^= *in_ptr++;
  }

  return checksum;

}



/* get a buffer to use for multi-line sentence decoding */
std::unique_ptr<Buffer> MultiSentenceBufferStore::getBuffer()
{
  if (m_buffers.empty() == true)
  {
    return std::unique_ptr<Buffer>(new Buffer (0));
  }
  else
  {
    auto pBuf = std::move(m_buffers.back());
    m_buffers.pop_back();
    return pBuf;
  }
}

/* return buffer to pool */
void MultiSentenceBufferStore::returnBuffer(std::unique_ptr<Buffer> &_buffer)
{
  _buffer->clear();
  m_buffers.push_back(std::move(_buffer));
}



MultiSentence::MultiSentence(int _iFragmentCount, const StringRef &_strFragment,
                             const StringRef &_strLine, const StringRef &_strHeader,
                             const StringRef &_strFooter,
                             MultiSentenceBufferStore &_bufferStore)
  : m_iFragmentCount(_iFragmentCount),
    m_iFragmentNum(0),
    m_vecLines(_iFragmentCount),
    m_bufferStore(_bufferStore)
{
  // buffer payload
  m_pPayloadBuffer = m_bufferStore.getBuffer();
  m_strPayload = bufferString(m_pPayloadBuffer, _strFragment);

  // buffer header and footer
  m_strHeader = bufferString(_strHeader);
  m_strFooter = bufferString(_strFooter);

  // init first fragment
  m_vecLines[0] = bufferString(_strLine);
  m_iFragmentNum++;
}

MultiSentence::~MultiSentence()
{
  // return buffers
  m_bufferStore.returnBuffer(m_pPayloadBuffer);

  for (auto &pBuf : m_metaBuffers)
  {
    m_bufferStore.returnBuffer(pBuf);
  }
}

bool MultiSentence::addFragment(int _iFragmentNum, const StringRef &_strFragment, const StringRef &_strLine)
{
  // check that fragments are added in order (out of order is an error)
  if ( (_iFragmentNum <= m_iFragmentCount) &&
       (m_iFragmentNum == _iFragmentNum - 1) )
  {
    // append data
    m_strPayload = bufferString(m_pPayloadBuffer, _strFragment);
    m_vecLines[_iFragmentNum - 1] = bufferString(_strLine);
    m_iFragmentNum++;

    return true;
  }
  else
  {
    return false;
  }
}

bool MultiSentence::isComplete() const
{
  return m_iFragmentCount == m_iFragmentNum;
}

/* copies string view into internal buffer (adds to buffer) */
StringRef MultiSentence::bufferString(const std::unique_ptr<Buffer> &_pBuffer, const StringRef &_str)
{
  // append to buffer and return new string ref to whole buffer
  _pBuffer->append(_str.data(), _str.size());
  return StringRef(_pBuffer->data(), _pBuffer->size());
}

/* copies string view into internal buffer (creates new buffer) */
StringRef MultiSentence::bufferString(const StringRef &_str)
{
  // get new buffer and append data
  m_metaBuffers.push_back(m_bufferStore.getBuffer());
  auto &pMetaBuffer = m_metaBuffers.back();
  pMetaBuffer->append(_str.data(), _str.size());

  // return new string ref to buffer
  return StringRef(pMetaBuffer->data(), _str.size());
}



AisDecoder::AisDecoder(int _iIndex)
  : m_iIndex(_iIndex),
    m_multiSentences{},
    m_msgCounts{},
    m_uTotalMessages(0),
    m_uTotalBytes(0),
    m_uCrcErrors(0),
    m_uDecodingErrors(0),
    m_vecMsgCallbacks{}
{
  enableMsgTypes({});
}


/* enable/disable msg callback */
void AisDecoder::setMsgCallback(int _iType, pfnMsgCallback _pfnCb, bool _bEnabled)
{
  if ( (_iType >= 0) &&
       (_iType < (int)m_vecMsgCallbacks.size()) )
  {
    if (_bEnabled == true)
    {
      m_vecMsgCallbacks[_iType] = _pfnCb;
    }
    else
    {
      m_vecMsgCallbacks[_iType] = nullptr;
    }
  }
}

/* enable/disable msg callback */
void AisDecoder::setMsgCallback(int _iType, pfnMsgCallback _pfnCb, const std::set<int> &_enabledTypes)
{
  // NOTE: some callbacks attach to multiple message types
  setMsgCallback(_iType, _pfnCb, (_enabledTypes.empty() == true) || (_enabledTypes.count(_iType) > 0));
}

/*
  Enables which messages types to decode.
  An empty set will enable all message types.
*/
void AisDecoder::enableMsgTypes(const std::set<int> &_types)
{
  setMsgCallback(1, &AisDecoder::decodeType123, _types);
  setMsgCallback(2, &AisDecoder::decodeType123, _types);
  setMsgCallback(3, &AisDecoder::decodeType123, _types);
  setMsgCallback(4, &AisDecoder::decodeType411, _types);
  setMsgCallback(5, &AisDecoder::decodeType5, _types);
  setMsgCallback(9, &AisDecoder::decodeType9, _types);
  setMsgCallback(11, &AisDecoder::decodeType411, _types);
  setMsgCallback(14, &AisDecoder::decodeType14, _types);
  setMsgCallback(18, &AisDecoder::decodeType18, _types);
  setMsgCallback(19, &AisDecoder::decodeType19, _types);
  setMsgCallback(21, &AisDecoder::decodeType21, _types);
  setMsgCallback(24, &AisDecoder::decodeType24, _types);
  setMsgCallback(27, &AisDecoder::decodeType27, _types);
}

/* decode Position Report (class A) */
void AisDecoder::decodeType123(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 168)
  {
    throw std::runtime_error("Invalid payload size.");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  auto Repeat = _buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  auto navstatus = _buffer.getUnsignedValue(4);
  auto rot = _buffer.getSignedValue(8);
  auto sog = _buffer.getUnsignedValue(10);
  auto posAccuracy = _buffer.getBoolValue();
  auto posLon = (long)_buffer.getSignedValue(28);
  auto posLat = (long)_buffer.getSignedValue(27);
  auto cog = (int)_buffer.getUnsignedValue(12);
  auto heading = (int)_buffer.getUnsignedValue(9);
  auto timestamp = _buffer.getUnsignedValue(6);     // timestamp
  auto maneuver_i = _buffer.getUnsignedValue(2);     // maneuver indicator
  _buffer.getUnsignedValue(3);     // spare
  auto Raim = _buffer.getBoolValue();          // RAIM
  _buffer.getUnsignedValue(19);     // radio status

  onType123(_uMsgType, mmsi, navstatus, rot, sog, posAccuracy, posLon, posLat, cog, heading, Repeat, Raim, timestamp, maneuver_i);
}

/* decode Base Station Report (type nibble already pulled from buffer) */
void AisDecoder::decodeType411(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 168)
  {
    throw std::runtime_error("Invalid payload size.");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  _buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  auto year = _buffer.getUnsignedValue(14);    // year UTC, 1-9999, 0 = N/A
  auto month = _buffer.getUnsignedValue(4);    // month (1-12), 0 = N/A
  auto day = _buffer.getUnsignedValue(5);      // day (1-31), 0 = N/A
  auto hour = _buffer.getUnsignedValue(5);     // hour (0 - 23), 24 = N/A
  auto minute = _buffer.getUnsignedValue(6);   // minute (0-59), 60 = N/A
  auto second = _buffer.getUnsignedValue(6);   // second 0-59, 60 = N/A
  auto posAccuracy = _buffer.getBoolValue();
  auto posLon = _buffer.getSignedValue(28);
  auto posLat = _buffer.getSignedValue(27);

  _buffer.getUnsignedValue(4);     // epfd type
  _buffer.getUnsignedValue(10);    // spare
  _buffer.getBoolValue();          // RAIM
  _buffer.getUnsignedValue(19);    // radio status

  onType411(_uMsgType, mmsi, year, month, day, hour, minute, second, posAccuracy, posLon, posLat);
}

/* decode Voyage Report (type nibble already pulled from buffer) */
void AisDecoder::decodeType5(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 420)
  {
    throw std::runtime_error("Invalid payload size.");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  auto repeat = _buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  auto ais_version = _buffer.getUnsignedValue(2);                 // AIS version
  auto imo = _buffer.getUnsignedValue(30);
  auto callsign = _buffer.getString(42);
  auto name = _buffer.getString(120);
  auto type = _buffer.getUnsignedValue(8);
  if (type > 99) {
    type = 0;
  }

  auto toBow = _buffer.getUnsignedValue(9);
  auto toStern = _buffer.getUnsignedValue(9);
  auto toPort = _buffer.getUnsignedValue(6);
  auto toStarboard = _buffer.getUnsignedValue(6);
  auto fixType = _buffer.getUnsignedValue(4);
  auto etaMonth = _buffer.getUnsignedValue(4);    // month (1-12), 0 = N/A
  auto etaDay = _buffer.getUnsignedValue(5);      // day (1-31), 0 = N/A
  auto etaHour = _buffer.getUnsignedValue(5);     // hour (0 - 23), 24 = N/A
  auto etaMinute = _buffer.getUnsignedValue(6);   // minute (0-59), 60 = N/A
  auto draught = _buffer.getUnsignedValue(8);
  auto destination = _buffer.getString(120);

  auto dte = _buffer.getBoolValue();                         // dte
  _buffer.getUnsignedValue(1);                    // spare

  onType5(_uMsgType, mmsi, imo, callsign, name, type, toBow, toStern, toPort, toStarboard, fixType, etaMonth, 
  etaDay, etaHour, etaMinute, draught, destination, ais_version, repeat, dte);
}

/* decode Standard SAR Aircraft Position Report */
void AisDecoder::decodeType9(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 168)
  {
    throw std::runtime_error("Invalid payload size.");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  _buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  auto altitude = _buffer.getUnsignedValue(12);
  auto sog = _buffer.getUnsignedValue(10);
  auto posAccuracy = _buffer.getBoolValue();
  auto posLon = _buffer.getSignedValue(28);
  auto posLat = _buffer.getSignedValue(27);
  auto cog = (int)_buffer.getUnsignedValue(12);

  _buffer.getUnsignedValue(6);     // timestamp
  _buffer.getUnsignedValue(8);     // reserved
  _buffer.getBoolValue();          // dte
  _buffer.getUnsignedValue(3);     // spare
  _buffer.getBoolValue();          // assigned
  _buffer.getBoolValue();          // RAIM
  _buffer.getUnsignedValue(20);    // radio status

  onType9(mmsi, sog, posAccuracy, posLon, posLat, cog, altitude);
}


/* decode AIS safety related broadcast */
void AisDecoder::decodeType14(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (!(_iPayloadSizeBits > 40 && _iPayloadSizeBits < 1009))
  {
    throw std::runtime_error("Invalid payload size.");
  }
  
  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  auto repeat =_buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  _buffer.getUnsignedValue(2);  
  auto text = _buffer.getString(_iPayloadSizeBits - 34 );
  onType14(repeat, mmsi, text, _iPayloadSizeBits);
}


/* decode Position Report (class B; type nibble already pulled from buffer) */
void AisDecoder::decodeType18(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 168)
  {
    throw std::runtime_error("Invalid payload size.");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  auto repeat = _buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  _buffer.getUnsignedValue(8);                 // reserved
  auto sog = _buffer.getUnsignedValue(10);
  auto posAccuracy = _buffer.getBoolValue();
  auto posLon = (long)_buffer.getSignedValue(28);
  auto posLat = (long)_buffer.getSignedValue(27);
  auto cog = (int)_buffer.getUnsignedValue(12);
  auto heading = (int)_buffer.getUnsignedValue(9);

  auto timestamp = _buffer.getUnsignedValue(6);     // timestamp
  _buffer.getUnsignedValue(2);     // reserved
  auto unit = _buffer.getBoolValue();          // cs unit
  auto display = _buffer.getBoolValue();          // display
  auto dsc = _buffer.getBoolValue();          // dsc
  auto band = _buffer.getBoolValue();          // band
  auto msg22 = _buffer.getBoolValue();          // msg22
  auto assigned = _buffer.getBoolValue();          // assigned
  auto raim = _buffer.getBoolValue();          // RAIM
  auto state =_buffer.getUnsignedValue(20);     // radio status

  onType18(_uMsgType, mmsi, sog, posAccuracy, posLon, posLat, cog, heading, raim, repeat, unit, 
    display, dsc, band, msg22, assigned, timestamp, state);
}

/* decode Position Report (class B; type nibble already pulled from buffer) */
void AisDecoder::decodeType19(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 312)
  {
    throw std::runtime_error("Invalid payload size.");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  auto repeat = _buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  _buffer.getUnsignedValue(8);                 // reserved
  auto sog = _buffer.getUnsignedValue(10);
  auto posAccuracy = _buffer.getBoolValue();
  auto posLon = _buffer.getSignedValue(28);
  auto posLat = _buffer.getSignedValue(27);
  auto cog = (int)_buffer.getUnsignedValue(12);
  auto heading = (int)_buffer.getUnsignedValue(9);
  auto timestamp = _buffer.getUnsignedValue(6);                 // timestamp
  _buffer.getUnsignedValue(4);                 // reserved
  auto name = _buffer.getString(120);
  auto type = _buffer.getUnsignedValue(8);
  if (type > 99) {
    type = 0;
  }

  auto toBow = _buffer.getUnsignedValue(9);
  auto toStern = _buffer.getUnsignedValue(9);
  auto toPort = _buffer.getUnsignedValue(6);
  auto toStarboard = _buffer.getUnsignedValue(6);

  auto fixtype = _buffer.getUnsignedValue(4);     // fix type
  auto raim = _buffer.getBoolValue();          // RAIM
  auto dte = _buffer.getBoolValue();          // dte
  auto assigned = _buffer.getBoolValue();          // assigned
  _buffer.getUnsignedValue(4);     // spare

  onType19(mmsi, sog, posAccuracy, posLon, posLat, cog, 
  heading, name, type, toBow, toStern, toPort, toStarboard,
  timestamp, fixtype, dte, assigned, repeat, raim);
}

/* decode Aid-to-Navigation Report */
void AisDecoder::decodeType21(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 272)
  {
    throw std::runtime_error("Invalid payload size.");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  auto repeat=_buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  auto aidType = _buffer.getUnsignedValue(5);
  auto name = _buffer.getString(120);
  auto posAccuracy = _buffer.getBoolValue();
  auto posLon = _buffer.getSignedValue(28);
  auto posLat = _buffer.getSignedValue(27);
  auto toBow = _buffer.getUnsignedValue(9);
  auto toStern = _buffer.getUnsignedValue(9);
  auto toPort = _buffer.getUnsignedValue(6);
  auto toStarboard = _buffer.getUnsignedValue(6);

  _buffer.getUnsignedValue(4);        // epfd type
  auto timestamp=_buffer.getUnsignedValue(6);        // timestamp
  auto offPosition=_buffer.getBoolValue();             // off position
  _buffer.getUnsignedValue(8);        // reserved
  auto raim=_buffer.getBoolValue();             // RAIM
  auto virtualAton=_buffer.getBoolValue();             // virtual aid
  _buffer.getBoolValue();             // assigned mode
  _buffer.getUnsignedValue(1);        // spare

  std::string nameExt;
  if (_iPayloadSizeBits > 272)
  {
    nameExt = _buffer.getString(88);
  }

  onType21(mmsi, aidType, name + nameExt, posAccuracy, posLon, posLat, 
    toBow, toStern, toPort, toStarboard,
    repeat,timestamp, raim, virtualAton, offPosition);
}

/* decode Voyage Report and Static Data (type nibble already pulled from buffer) */
void AisDecoder::decodeType24(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  auto repeat =_buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  auto partNo = _buffer.getUnsignedValue(2);

  // decode part A
  if (partNo == 0)
  {
    if (_iPayloadSizeBits < 160)
    {
      throw std::runtime_error("Invalid payload size.");
    }

    auto name = _buffer.getString(120);
    _buffer.getUnsignedValue(8);            // spare

    onType24A(_uMsgType, repeat, mmsi, name);
  }

  // decode part B
  else if (partNo == 1)
  {
    if (_iPayloadSizeBits < 168)
    {


      throw std::runtime_error("Invalid payload size.");
    }

    auto type = _buffer.getUnsignedValue(8);
    if (type > 99) {
      type = 0;
    }

    auto vendor = _buffer.getString(18);         // vendor ID
    _buffer.getUnsignedValue(4);                 // unit model code
    _buffer.getUnsignedValue(20);                // serial number
    auto callsign = _buffer.getString(42);
    auto toBow = _buffer.getUnsignedValue(9);
    auto toStern = _buffer.getUnsignedValue(9);
    auto toPort = _buffer.getUnsignedValue(6);
    auto toStarboard = _buffer.getUnsignedValue(6);

    // FvdB: Note that 'Mothership MMSI' field overlaps the previous 4 fields, total message length is 168 bits
    // _buffer.getUnsignedValue(30);                // Mothership MMSI

    _buffer.getUnsignedValue(6);        // spare

    onType24B(_uMsgType, repeat, mmsi, callsign, type, toBow, toStern, toPort, toStarboard, vendor);
  }

  // invalid part
  else
  {
    throw std::runtime_error("Invalid part number.");
  }
}

/* decode Long Range AIS Broadcast message (type nibble already pulled from buffer) */
void AisDecoder::decodeType27(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits)
{
  if (_iPayloadSizeBits < 96)
  {

    throw std::runtime_error("Invalid payload size");
  }

  // decode message fields (binary buffer has to go through all fields, but some fields are not used)
  _buffer.getUnsignedValue(2);                 // repeatIndicator
  auto mmsi = _buffer.getUnsignedValue(30);
  auto posAccuracy = _buffer.getBoolValue();

  _buffer.getBoolValue();         // RAIM

  auto navstatus = _buffer.getUnsignedValue(4);
  auto posLon = _buffer.getSignedValue(18);
  auto posLat = _buffer.getSignedValue(17);
  auto sog = _buffer.getUnsignedValue(6);
  auto cog = (int)_buffer.getUnsignedValue(9);

  _buffer.getUnsignedValue(1);        // GNSS
  _buffer.getUnsignedValue(1);        // spare

  onType27(mmsi, navstatus, sog, posAccuracy, posLon, posLat, cog);
}

/* decode Mobile AIS station message */
void AisDecoder::decodeMobileAisMsg(const StringRef &_strPayload, int _iFillBits)
{
  // de-armour string and back bits into buffer
  m_binaryBuffer.resetBitIndex();
  int iBitsUsed = decodeAscii(m_binaryBuffer, _strPayload, _iFillBits);
  m_binaryBuffer.resetBitIndex();

  // check message type
  auto msgType = m_binaryBuffer.getUnsignedValue(6);
  if ( (msgType == 0) ||
       (msgType > 27) )
  {


    onDecodeError(_strPayload, "Invalid message type.");
  }
  else
  {
    // decode message
    m_msgCounts[msgType]++;
    m_uTotalMessages++;

    auto pFnDecoder = m_vecMsgCallbacks[msgType];
    if (pFnDecoder != nullptr)
    {
      (this->*pFnDecoder)(m_binaryBuffer, msgType, iBitsUsed);
    }
    else
    {
      onNotDecoded(_strPayload, msgType);
    }
  }
}

/* check sentence CRC */
bool AisDecoder::checkCrc(const StringRef &_strPayload)
{
  if (_strPayload.size() > 3)
  {
    const char *pCrc = _strPayload.data() + _strPayload.size() - 3;
    if (*pCrc == '*')
    {
      // for some reason the std::strol function is quite slow, so just conver the checksum manually
      const uint16_t ascii_t[32] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        257, 257, 257, 257, 257, 257, 257,
        10, 11, 12, 13, 14, 15,
        257, 257, 257, 257, 257, 257, 257, 257, 257
      };

      uint16_t iCrc = (ascii_t[(*(pCrc + 1) - '0') & 31] << 4) +
                      ascii_t[(*(pCrc + 2) - '0') & 31];

      const char pCh = *_strPayload.data();
      if ( (pCh == '!') ||
           (pCh == '$') )
      {
        uint16_t iCrcCalc = (int)AIS::crc(StringRef(_strPayload.data() + 1, _strPayload.size() - 4));
        return iCrc == iCrcCalc;
      }
      else
      {
        uint16_t iCrcCalc = (int)AIS::crc(StringRef(_strPayload.data(), _strPayload.size() - 3));
        return iCrc == iCrcCalc;
      }
    }
  }

  return false;
}

/* check talker id */
bool AisDecoder::checkTalkerId(const StringRef &_strTalkerId)
{
  if (_strTalkerId.size() > 2)
  {
    char chA = _strTalkerId.data()[0];
    if ( (chA == '!') ||
         (chA == '$') )
    {
      chA = _strTalkerId.data()[1];
    }

    return (chA == 'A') || (chA == 'B');
  }
  else
  {
    return false;
  }
}

/*
  Decode next sentence (starts reading from input buffer with the specified offset; returns the number of bytes processed, or 0 when no more messages can be decoded).
  Has to be called until it returns 0, to ensure that any buffered multi-line strings are backed up properly.
*/
size_t AisDecoder::decodeMsg(const char *_pNmeaBuffer, size_t _uBufferSize, size_t _uOffset, 
  const SentenceParser &_parser, bool treatAsComplete)
{
  // process and decode AIS strings
  StringRef strLine;
  size_t n = 0;
  if (treatAsComplete){
    strLine=StringRef(_pNmeaBuffer+_uOffset,_uBufferSize);
  } 
  else{
    n=getLine(strLine, _pNmeaBuffer, _uBufferSize, _uOffset);
  }
  if (strLine.size() > 2)      // ignore empty lines
  {
    // clear user data
    m_strHeader = StringRef();
    m_strFooter = StringRef();
    m_strPayload = StringRef();
    m_vecSentences.clear();

    // provide raw data back to user
    onSentence(strLine);

    // pull out NMEA sentence
    // NOTE: META header and footer will be calculated from position and size of NMEA sentence within last line
    auto strNmea = _parser.onScanForNmea(strLine);
    if (strNmea.empty() == false)
    {
      // check sentence CRC
      if (checkCrc(strNmea) == true)
      {
        // decode sentence
        size_t uWordCount = seperate<','>(m_words, strNmea);
        bool bValidMsg = (checkTalkerId(m_words[0]) == true) &&
                         (uWordCount >= 7) &&
                         (m_words[5].size() > 1) &&
                         (m_words[5].size() <= MAX_MSG_PAYLOAD_LENGTH);

        // try to decode sentence
        if (bValidMsg == true)
        {

          int iFragmentCount = single_digit_strtoi(m_words[1]);
          int iFillBits = single_digit_strtoi(m_words[6]);

          // check for valid sentence
          if ( (iFragmentCount == 0) || (iFragmentCount > MAX_MSG_FRAGMENTS) )
          {
            m_uDecodingErrors++;

            onDecodeError(strNmea, "Invalid fragment count value.");
          }

          // decode simple sentence
          else if (iFragmentCount == 1)
          {
            // setup user data
            m_strHeader = _parser.getHeader(strLine, strNmea);
            m_strFooter = _parser.getFooter(strLine, strNmea);
            m_strPayload = m_words[5];
            m_vecSentences.push_back(strLine);

            try
            {
              // NOTE: the order of the callbacks is important (supply RAW/META info first then decode message)
              onMessage(m_strPayload, m_strHeader, m_strFooter);
              decodeMobileAisMsg(m_strPayload, iFillBits);
            }
            catch (std::exception &ex)
            {
              m_uDecodingErrors++;
              onDecodeError(m_strPayload, ex.what());
            }
          }

          // build up multi-sentence payloads
          else // if (iFragmentCount > 1)
          {
            int iMsgId = strtoi(m_words[3]);
            int iFragmentNum = single_digit_strtoi(m_words[2]);

            // check for valid message
            if (iMsgId >= (int)m_multiSentences.size())
            {
              m_uDecodingErrors++;
              onDecodeError(strNmea, "Invalid message sequence id.");
            }

            // check for valid message
            else if ( (iFragmentNum == 0) || (iFragmentNum > iFragmentCount) )
            {
              m_uDecodingErrors++;
              onDecodeError(strNmea, "Invalid message fragment number.");
            }

            // create multi-sentence object with first message
            else if (iFragmentNum == 1)
            {
              m_multiSentences[iMsgId] = std::unique_ptr<MultiSentence>(new MultiSentence(iFragmentCount, m_words[5], strLine,
                                         _parser.getHeader(strLine, strNmea),
                                         _parser.getFooter(strLine, strNmea),
                                         m_multiSentenceBuffers));
            }

            // update multi-sentence object with more fragments
            else
            {
              // add to existing payload
              auto &pMultiSentence = m_multiSentences[iMsgId];
              if (pMultiSentence != nullptr)
              {
                // add new fragment and check for any message payload/fragment errors
                bool bSuccess = pMultiSentence->addFragment(iFragmentNum, m_words[5], strLine);

                if (bSuccess == true)
                {
                  // check if all fragments have been received
                  if (pMultiSentence->isComplete() == true)
                  {
                    // setup user data
                    m_strHeader = pMultiSentence->header();
                    m_strFooter = pMultiSentence->footer();
                    m_vecSentences = pMultiSentence->sentences();
                    m_strPayload = pMultiSentence->payload();

                    // decode whole payload and reset
                    try
                    {
                      // NOTE: the order of the callbacks is important (supply RAW/META info first, then decode message)
                      // NOTE: only uses META info of first line for multi-line messages
                      onMessage(m_strPayload, m_strHeader, m_strFooter);
                      decodeMobileAisMsg(m_strPayload, iFillBits);
                    }
                    catch (std::exception &ex)
                    {
                      m_uDecodingErrors++;
                      onDecodeError(m_strPayload, ex.what());
                    }

                    // cleanup
                    m_multiSentences[iMsgId] = nullptr;
                  }
                }
                else
                {
                  // sentence error, so just reset
                  m_uDecodingErrors++;
                  m_multiSentences[iMsgId] = nullptr;
                  onDecodeError(strNmea, "Multi-sentence decoding failed.");
                }
              }
              else
              {
                // sentence error, so just reset
                m_uDecodingErrors++;
                m_multiSentences[iMsgId] = nullptr;
                onDecodeError(strNmea, "Multi-sentence decoding failed.");
              }
            }
          }
        }
        else
        {
          m_uDecodingErrors++;
          onDecodeError(strNmea, "Sentence not a valid AIS sentence.");
        }
      }
      else
      {
        m_uCrcErrors++;
        onDecodeError(strNmea, "Sentence decoding error. CRC check failed.");
      }
    }
    else
    {
      onParseError(strLine, "Sentence decoding error. No valid NMEA data found.");
    }

    m_uTotalBytes += n;
  }

  return n;
}
