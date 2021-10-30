#include "NMEA0183DataToN2K.h"
#include "NMEA0183Messages.h"
NMEA0183DataToN2K::NMEA0183DataToN2K(GwLog *logger, GwBoatData *boatData,N2kSender callback)
{
    this->sender = callback;
    this->logger = logger;
    this->boatData=boatData;
    LOG_DEBUG(GwLog::LOG,"NMEA0183DataToN2K created %p",this);
}

bool NMEA0183DataToN2K::parseAndSend(const char *buffer, int sourceId) {
    LOG_DEBUG(GwLog::DEBUG,"NMEA0183DataToN2K[%d] parsing %s",sourceId,buffer)
    return false;
}


class NMEA0183DataToN2KFunctions : public NMEA0183DataToN2K{
    public:
        typedef void (NMEA0183DataToN2KFunctions::*Converter)(const tNMEA0183Msg  &msg);
    private:    
        class ConverterEntry
        {
            public:
                unsigned long count = 0;
                unsigned long *pgn;
                unsigned int numPgn=0;
                Converter converter;
                ConverterEntry(){
                    pgn=NULL;
                    converter=NULL;
                }
                ConverterEntry(unsigned long pgn,Converter cv = NULL) {
                     converter = cv; 
                     numPgn=1;
                     this->pgn=new unsigned long[1];
                     this->pgn[0]=pgn;
                     }
                ConverterEntry(unsigned long pgn1,unsigned long pgn2,Converter cv = NULL) {
                     converter = cv; 
                     numPgn=2;
                     this->pgn=new unsigned long[2];
                     this->pgn[0]=pgn1;
                     this->pgn[1]=pgn2;
                     }
                ConverterEntry(unsigned long pgn1,unsigned long pgn2,unsigned long pgn3,Converter cv = NULL) {
                     converter = cv; 
                     numPgn=3;
                     this->pgn=new unsigned long[3];
                     this->pgn[0]=pgn1;
                     this->pgn[1]=pgn2;
                     this->pgn[2]=pgn3;
                     }          
        };
        typedef std::map<String, ConverterEntry> ConverterMap;
        ConverterMap converters;

        /**
        *  register a converter
        *  each of the converter functions must be registered in the constructor 
        **/
        void registerConverter(unsigned long pgn, String sentence,Converter converter)
        {
            ConverterEntry e(pgn,converter);
            converters[sentence] = e;
        }
        void registerConverter(unsigned long pgn,unsigned long pgn2, String sentence,Converter converter)
        {
            ConverterEntry e(pgn,pgn2,converter);
            converters[sentence] = e;
        }

        void convertRMB(const tNMEA0183Msg &msg){
            LOG_DEBUG(GwLog::DEBUG+1,"convert RMB");
        }
    public:
        virtual bool parseAndSend(const char *buffer, int sourceId) {
            LOG_DEBUG(GwLog::DEBUG+1,"NMEA0183DataToN2K[%d] parsing %s",sourceId,buffer)
            tNMEA0183Msg msg;
            if (! msg.SetMessage(buffer)){
                LOG_DEBUG(GwLog::DEBUG,"NMEA0183DataToN2K[%d] invalid message %s",sourceId,buffer)
                return false;
            }
            String code=String(msg.MessageCode());
            ConverterMap::iterator it=converters.find(code);
            if (it != converters.end()){
                (it->second).count++;
                //call to member function - see e.g. https://isocpp.org/wiki/faq/pointers-to-members
                ((*this).*((it->second).converter))(msg);   
            }
            else{
                LOG_DEBUG(GwLog::DEBUG,"NMEA0183DataToN2K[%d] no handler for  %s",sourceId,code.c_str());
                return false;
            }
            return true;
        }    
        virtual unsigned long *handledPgns()
        {
            logger->logString("CONV: # %d handled PGNS", (int)converters.size());
            //for now max 3 pgns per converter
            unsigned long *rt = new unsigned long[converters.size() *3 + 1];
            int idx = 0;
            for (ConverterMap::iterator it = converters.begin();
                it != converters.end(); it++)
            {   
                for (int i=0;i<it->second.numPgn && i < 3;i++){
                    bool found=false;
                    for (int e=0;e<idx;e++){
                        if (rt[e] == it->second.pgn[i]){
                            found=true;
                            break;
                        }
                    }
                    if (! found){
                        rt[idx] = it->second.pgn[i];
                        idx++;
                    }
                }
            }
            rt[idx] = 0;
            return rt;
        }
        NMEA0183DataToN2KFunctions(GwLog *logger,GwBoatData *boatData,N2kSender callback)
            :NMEA0183DataToN2K(logger,boatData,callback){
                registerConverter(129283UL,String(F("RMB")),&NMEA0183DataToN2KFunctions::convertRMB);
            }
  
};

NMEA0183DataToN2K* NMEA0183DataToN2K::create(GwLog *logger,GwBoatData *boatData,N2kSender callback){
    return new NMEA0183DataToN2KFunctions(logger, boatData,callback);

}
