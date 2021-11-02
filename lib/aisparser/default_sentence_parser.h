
#ifndef TOOLKIT_AIS_DEFAULT_PARSER_H
#define TOOLKIT_AIS_DEFAULT_PARSER_H


#include "ais_decoder.h"



namespace AIS
{
    /**
     Default Sentence Parser
     
     This implementation will scan past META data that start and end with a '\'.  It will also stop at NMEA CRC.
     The META data footer and header are calculated based on the start and the end of the NMEA string in each sentence.
     
     */
    class DefaultSentenceParser     : public SentenceParser
    {
     public:
        /// called to find NMEA start (scan past any headers, META data, etc.; returns NMEA payload)
        virtual StringRef onScanForNmea(const StringRef &_strSentence) const override;

        /// calc header string from original line and extracted NMEA payload
        virtual StringRef getHeader(const StringRef &_strLine, const StringRef &_strNmea) const override;
        
        /// calc footer string from original line and extracted NMEA payload
        virtual StringRef getFooter(const StringRef &_strLine, const StringRef &_strNmea) const override;
        
        /// extracts the timestamp from the meta info
        virtual uint64_t getTimestamp(const AIS::StringRef &_strHeader, const AIS::StringRef &_strFooter) const override;

    };
    
    
};  // namespace AIS


#endif        // #ifndef TOOLKIT_AIS_DEFAULT_PARSER_H
