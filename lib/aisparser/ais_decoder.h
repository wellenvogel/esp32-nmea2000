
#ifndef TOOLKIT_AIS_DECODER_H
#define TOOLKIT_AIS_DECODER_H


#include "strutils.h"

#include <string>
#include <vector>
#include <array>
#include <set>



namespace AIS
{
    // constants
    const char ASCII_CHARS[]                = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?";
    const size_t MAX_FRAGMENTS              = 5;
    const size_t MAX_CHARS_PER_FRAGMENT     = 82;
    
    
    /**
     Buffer used to push back 6 bit nibbles and extract as signed, unsigned, boolean and string values
     */
    class PayloadBuffer
    {
     private:
        const static int MAX_PAYLOAD_SIZE = MAX_FRAGMENTS * MAX_CHARS_PER_FRAGMENT * 6 / 8 + 1;
        
     public:
        PayloadBuffer();
        
        /// set bit index back to zero
        void resetBitIndex();
        
        /// unpack next _iBits (most significant bit is packed first)
        unsigned int getUnsignedValue(int _iBits);
        
        /// unpack next _iBits (most significant bit is packed first; with sign check/conversion)
        int getSignedValue(int _iBits);
        
        /// unback next boolean (1 bit)
        bool getBoolValue()
        {
            return getUnsignedValue(1) != 0;
        }

        /// unback string (6 bit characters) -- already cleans string (removes trailing '@' and trailing spaces)
        std::string getString(int _iNumBits);
        
        unsigned char* getData(void) {
            return &m_data[0];
        }
        
     private:
        alignas(16) std::array<unsigned char, MAX_PAYLOAD_SIZE>       m_data;
        int32_t                                                       m_iBitIndex;
    };
    
    
    /// Convert payload to decimal (de-armour) and concatenate 6bit decimal values into payload buffer. Returns the number of bits used.
    int decodeAscii(PayloadBuffer &_buffer, const StringRef &_strPayload, int _iFillBits);
    
    /// calc CRC
    uint8_t crc(const StringRef &_strLine);
    
    
    
    /**
     Multi-sentence messages migth span across different source buffers and input (string views) has to be stored internally.
     This class is a store of buffers for use by multi-sentence containers (see MultiSentence).
     
     */
    class MultiSentenceBufferStore
    {
     public:
        /// get a buffer to use for multi-line sentence decoding
        std::unique_ptr<Buffer> getBuffer();
        
        /// return buffer to pool
        void returnBuffer(std::unique_ptr<Buffer> &_buffer);

     private:
        std::vector<std::unique_ptr<Buffer>>    m_buffers;      ///< available buffers
    };
    
    
    /**
     Multi-sentence message container.
     
     Multi-sentence messages migth span across different source buffers and the input has to be stored internally.
     A buffer pool (see MultiSentenceBufferStore) is used to avoid memory allocations as far as possible when storing the
     message data.
     
     */
    class MultiSentence
    {
     public:
        /// constructor -- add first fragment
        MultiSentence(int _iFragmentCount, const StringRef &_strFragment,
                      const StringRef &_strLine,
                      const StringRef &_strHeader, const StringRef &_strFooter,
                      MultiSentenceBufferStore &_bufferStore);
        
        ~MultiSentence();
        
        /// add more fragments (returns false if there is an fragment indexing error)
        bool addFragment(int _iFragmentNum, const StringRef &_strFragment, const StringRef &_strLine);
        
        /// returns true if all fragments have been received
        bool isComplete() const;
        
        /// returns full payload
        const StringRef &payload() const {return m_strPayload;}
        
        /// returns full payload (ref stays valid only while this object exists and addFragment is not called)
        const StringRef &header() const {return m_strHeader;}
        
        /// returns full payload (ref stays valid only while this object exists and addFragment is not called)
        const StringRef &footer() const {return m_strFooter;}
        
        /// returns original sentences referenced my this multi-line sentence
        const std::vector<StringRef> &sentences() const {return m_vecLines;}
        
     private:
        /// copies string view into internal buffer (adds to buffer)
        StringRef bufferString(const std::unique_ptr<Buffer> &_pBuffer, const StringRef &_str);
        
        /// copies string view into internal buffer (creates new buffer)
        StringRef bufferString(const StringRef &_str);

     protected:
        int                                     m_iFragmentCount;
        int                                     m_iFragmentNum;
        StringRef                               m_strHeader;
        StringRef                               m_strFooter;
        StringRef                               m_strPayload;
        std::vector<StringRef>                  m_vecLines;
        MultiSentenceBufferStore                &m_bufferStore;
        std::vector<std::unique_ptr<Buffer>>    m_metaBuffers;
        std::unique_ptr<Buffer>                 m_pPayloadBuffer;
    };
    
    
    
    /**
     Sentence Parser base class.
     This class can be extended to pull out NMEA string and META info from custom sentence formats.
     
     The simplest implementation for this class would just return any source sentence as the full NMEA sentence:
        - 'onScanForNmea' just returns the input parameter
        - 'getHeader' returns an empty string
        - 'getFooter' returns an empty string
        - 'getTimestamp' returns 0

     */
    class SentenceParser
    {
     public:
        /// called to find NMEA start (scan past any headers, META data, etc.; returns NMEA payload; may be overloaded for app specific meta data)
        virtual StringRef onScanForNmea(const StringRef &_strSentence) const = 0;

        /// calc header string from original line and extracted NMEA payload
        virtual StringRef getHeader(const StringRef &_strLine, const StringRef &_strNmea) const = 0;
        
        /// calc footer string from original line and extracted NMEA payload
        virtual StringRef getFooter(const StringRef &_strLine, const StringRef &_strNmea) const = 0;
        
        /// extracts the timestamp from the meta info (should return 0 if no timestamp found)
        virtual uint64_t getTimestamp(const AIS::StringRef &_strHeader, const AIS::StringRef &_strFooter) const = 0;
    };
    
    
    
    /**
     AIS decoder base class.
     Implemented according to 'http://catb.org/gpsd/AIVDM.html'.
     
     To use the decoder, call 'decodeMsg(...)' with some data until it returns 0. 'decodeMsg(...)' returns the number of bytes processed and offset should be set on each call to point to the new
     location in the buffer.
     \note: 'decodeMsg(...)' has to be called until it returns 0, to ensure that any buffered multi-line strings are backed up properly.
     
     A user of the decoder has to inherit from the decoder class and implement/override 'onTypeXX(...)' style methods as well as error handling methods.
     Some user onTypeXX(...) methods are attached to multiple message types, for example: 123 (types 1, 2 & 3) and 411 (types 4 & 11), in which case the message type is the first parameter.
     
     Callback sequence:
        - onSentence(..) provides raw message fragments as they are received
        - onMessage(...) provides message payload and meta info of the message being decoded
        - onTypeXX(...) provides message specific callbacks
     
     Basic error checking, including CRC checks, are done and also reported.
     No assumtions are made on default or blank values -- all values are returned as integers and the user has to scale and convert the values like position and speed to floats and the desired units.
     
     The method 'enableMsgTypes(...)' can be used to enable/disable the decoding of specific messages. For example 'enableMsgTypes({1, 5})' will cause only type 1 and type 5 to be decoded internally, which could
     increase decoding performance, since the decoder will just skip over other message types.  The method takes a list or set of integers, for example '{1, 2, 3}' or '{5}'.
     
     A SentenceParser object, supplied as a parameter to 'decodeMsg(...)', allows the decoder to support custom META data around the NMEA sentences.
     For multiline messages only the header and footer of the first sentence is reported when decoding messages (reported via 'onMessage(...)').
     
     The decoder also provides access to the META and raw sentence data as messages are being decoded.
     The following methods can be called from inside the 'onMessage()', 'onTypeXX()' or 'onDecodeError()' methods:
     
     - 'header()' returns the extracted META data header
     - 'footer()' returns the extracted META data footer
     - 'payload()' returns the full NMEA payload
     - 'sentences()' returns list of original sentences that contributed
     
     */
    class AisDecoder
    {
     private:
        const static int MAX_MSG_SEQUENCE_IDS      = 10;       ///< max multi-sentience message sequences
        const static int MAX_MSG_TYPES             = 64;       ///< max message type count (unique messsage IDs)
        const static int MAX_MSG_PAYLOAD_LENGTH    = 82;       ///< max payload length (NMEA limit)
        const static int MAX_MSG_FRAGMENTS         = 5;        ///< maximum number of fragments/sentences a message can have
        const static int MAX_MSG_WORDS             = 10;       ///< maximum number of words per sentence
        
        using pfnMsgCallback = void (AisDecoder::*)(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);

     public:
        AisDecoder(int _iIndex = 0);
        
        /// returns the user defined index
        int index() const {return m_iIndex;}
        
        /**
            Enables which messages types to decode.
            An empty set will enable all message types.
         */
        void enableMsgTypes(const std::set<int> &_types);
        
        /**
            Decode next sentence (starts reading from input buffer with the specified offset; returns the number of bytes processed, or 0 when no more messages can be decoded).
            Has to be called until it returns 0, to ensure that any buffered multi-line strings are backed up properly.
         */
        size_t decodeMsg(const char *_pNmeaBuffer, size_t _uBufferSize, size_t _uOffset, 
            const SentenceParser &_parser, bool treatAsComplete=false);
        
        /// returns the total number of messages processed
        uint64_t getTotalMessageCount() const {return m_uTotalMessages;}
        
        /// returns the number of messages processed per type
        uint64_t getMessageCount(int _iMsgType) {return m_msgCounts[_iMsgType];}
        
        /// returns the total number of bytes processed
        uint64_t getTotalBytes() const {return m_uTotalBytes;}
        
        /// returns the number of CRC check errors
        uint64_t getCrcErrorCount() const {return m_uCrcErrors;}
        
        /// returns the total number of decoding errors
        uint64_t getDecodingErrorCount() const {return m_uDecodingErrors;}
     
        
        // access to message info
     protected:
        /// returns the META header of the current message (valid during calls to 'onMessage' and 'onTypeXX' callbacks)
        const StringRef &header() const {return m_strHeader;}
        
        /// returns the META footer of the current message (valid during calls to 'onMessage' and 'onTypeXX' callbacks)
        const StringRef &footer() const {return m_strFooter;}
        
        /// returns the full NMEA string of the current message (valid during calls to 'onMessage' and 'onTypeXX' callbacks)
        const StringRef &payload() const {return m_strPayload;}
        
        /// returns all the sentences that contributed to the current message (valid during calls to 'onMessage' and 'onTypeXX' callbacks)
        const std::vector<StringRef> &sentences() const {return m_vecSentences;}
        
        /// returns current message timestamp, decoded from message header or footer (returns 0 if none found)
        uint64_t timestamp() const;
        

        // user defined callbacks
     protected:
        virtual void onType123(unsigned int _uMsgType, unsigned int _uMmsi, unsigned int _uNavstatus, int _iRot, unsigned int _uSog, bool _bPosAccuracy, long _iPosLon, long _iPosLat, int _iCog, int _iHeading, int _Repeat, bool _Raim, unsigned int _timestamp, unsigned int _maneuver_i) = 0;
        virtual void onType411(unsigned int _uMsgType, unsigned int _uMmsi, unsigned int _uYear, unsigned int _uMonth, unsigned int _uDay, unsigned int _uHour, unsigned int _uMinute, unsigned int _uSecond,
                               bool _bPosAccuracy, int _iPosLon, int _iPosLat) = 0;
        virtual void onType5(unsigned int _uMsgType, unsigned int _uMmsi, unsigned int _uImo, const std::string &_strCallsign, const std::string &_strName,
                             unsigned int _uType, unsigned int _uToBow, unsigned int _uToStern, unsigned int _uToPort, unsigned int _uToStarboard, unsigned int _uFixType,
                             unsigned int _uEtaMonth, unsigned int _uEtaDay, unsigned int _uEtaHour, unsigned int _uEtaMinute, unsigned int _uDraught,
                             const std::string &_strDestination, unsigned int _ais_version, unsigned int _repeat, bool _dte) = 0;
        
        virtual void onType9(unsigned int _uMmsi, unsigned int _uSog, bool _bPosAccuracy, int _iPosLon, int _iPosLat, int _iCog, unsigned int _iAltitude) = 0;

        virtual void onType14(unsigned int _repeat, unsigned int _uMmsi, const std::string &_strText, int _iPayloadSizeBits) = 0;
        
        virtual void onType18(unsigned int _uMsgType, unsigned int _uMmsi, unsigned int _uSog, bool _bPosAccuracy, 
                               long _iPosLon, long _iPosLat, int _iCog, int _iHeading, bool _raim, unsigned int _repeat,
                               bool _unit, bool _diplay, bool _dsc, bool _band, bool _msg22, bool _assigned, 
                               unsigned int _timestamp, bool _state ) = 0;
        
        virtual void onType19(unsigned int _uMmsi, unsigned int _uSog, bool _bPosAccuracy, int _iPosLon, int _iPosLat, int _iCog, int _iHeading,
                              const std::string &_strName, unsigned int _uType,
                              unsigned int _uToBow, unsigned int _uToStern, unsigned int _uToPort, 
                              unsigned int _uToStarboard, unsigned int timestamp, unsigned int fixtype, bool dte, 
                              bool assigned, unsigned int repeat, bool raim) = 0;
        
        virtual void onType21(unsigned int _uMmsi, unsigned int _uAidType, const std::string &_strName, bool _bPosAccuracy, int _iPosLon, int _iPosLat,
                              unsigned int _uToBow, unsigned int _uToStern, unsigned int _uToPort, unsigned int _uToStarboard) = 0;
        
        virtual void onType24A(unsigned int _uMsgType, unsigned int _repeat, unsigned int _uMmsi, const std::string &_strName) = 0;
        
        virtual void onType24B(unsigned int _uMsgType, unsigned int _repeat, unsigned int _uMmsi, const std::string &_strCallsign, unsigned int _uType, unsigned int _uToBow, unsigned int _uToStern, unsigned int _uToPort, unsigned int _uToStarboard, const std::string &_strVendor) = 0;
        
        virtual void onType27(unsigned int _uMmsi, unsigned int _uNavstatus, unsigned int _uSog, bool _bPosAccuracy, int _iPosLon, int _iPosLat, int _iCog) = 0;
        
        /// called on every sentence (raw data) received (includes all characters, including NL, CR, etc.; called before any validation or CRCs checks are performed)
        virtual void onSentence(const StringRef &_strSentence) = 0;
        
        /// called on every full message, before 'onTypeXX(...)'
        virtual void onMessage(const StringRef &_strPayload, const StringRef &_strHeader, const StringRef &_strFooter) = 0;
        
        /// called when message type is not supported (i.e. 'onTypeXX(...)' not implemented)
        virtual void onNotDecoded(const StringRef &_strPayload, int _iMsgType) = 0;
        
        /// called when any decoding error ocurred
        virtual void onDecodeError(const StringRef &_strPayload, const std::string &_strError) = 0;
        
        /// called when any parsing error ocurred
        virtual void onParseError(const StringRef &_strLine, const std::string &_strError) = 0;
        
     private:
        /// enable/disable msg callback
        void setMsgCallback(int _iType, pfnMsgCallback _pfnCb, bool _bEnabled);
                            
        /// enable/disable msg callback
        void setMsgCallback(int _iType, pfnMsgCallback _pfnCb, const std::set<int> &_enabledTypes);
        
        /// check sentence CRC
        bool checkCrc(const StringRef &_strPayload);
        
        /// check talker id
        bool checkTalkerId(const StringRef &_strTalkerId);
        
        /// decode Mobile AIS station message
        void decodeMobileAisMsg(const StringRef &_strPayload, int _iFillBits);
        
        /// decode Position Report (class A; type nibble already pulled from buffer)
        void decodeType123(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Base Station Report (type nibble already pulled from buffer; or, response to inquiry)
        void decodeType411(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Voyage Report and Static Data (type nibble already pulled from buffer)
        void decodeType5(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Standard SAR Aircraft Position Report
        void decodeType9(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Standard SAR Aircraft Position Report
        void decodeType11(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);

        /// decode Safety related Broadcast Message
        void decodeType14(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Position Report (class B; type nibble already pulled from buffer)
        void decodeType18(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Position Report (class B; type nibble already pulled from buffer)
        void decodeType19(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Aid-to-Navigation Report
        void decodeType21(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Voyage Report and Static Data (type nibble already pulled from buffer)
        void decodeType24(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);
        
        /// decode Long Range AIS Broadcast message (type nibble already pulled from buffer)
        void decodeType27(PayloadBuffer &_buffer, unsigned int _uMsgType, int _iPayloadSizeBits);

    private:
        int                                                                     m_iIndex;               ///< arbitrary id/index set by user for this decoder
        
        PayloadBuffer                                                           m_binaryBuffer;         ///< used internally to decode NMEA payloads
        std::array<std::unique_ptr<MultiSentence>, MAX_MSG_SEQUENCE_IDS>        m_multiSentences;       ///< used internally to buffer multi-line message sentences
        std::array<StringRef, MAX_MSG_WORDS>                                    m_words;                ///< used internally to buffer NMEA words
        MultiSentenceBufferStore                                                m_multiSentenceBuffers;
        
        std::vector<StringRef>                                                  m_vecSentences;         ///< all NMEA/raw sentences for message - stored for each message just before user callbacks
        StringRef                                                               m_strHeader;            ///< extracted META header
        StringRef                                                               m_strFooter;            ///< extracted META header
        StringRef                                                               m_strPayload;           ///< extracted full payload (concatenated fragments; ascii)

        std::array<uint64_t, MAX_MSG_TYPES>                                     m_msgCounts;            ///< message counts per message type
        uint64_t                                                                m_uTotalMessages;
        uint64_t                                                                m_uTotalBytes;
        uint64_t                                                                m_uCrcErrors;           ///< CRC check error count
        uint64_t                                                                m_uDecodingErrors;      ///< decoding error count (includes CRC errors)
        
        std::array<pfnMsgCallback, 100>                                         m_vecMsgCallbacks;      ///< message decoding functions mapped to message IDs
    };
    
    
};  // namespace AIS


#endif        // #ifndef TOOLKIT_AIS_DECODER_H
