#include "NMEA0183DataToN2K.h"
#include "NMEA0183Messages.h"
#include "N2kMessages.h"
#include "ConverterList.h"
#include <map>
#include <strings.h>
#include "NMEA0183AIStoNMEA2000.h"
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

class SNMEA0183Msg : public tNMEA0183Msg{
        public:
            int sourceId;
            const char *line;
            bool isAis=false;
            SNMEA0183Msg(const char *line, int sourceId){
                this->sourceId=sourceId;
                this->line=line;
                int len=strlen(line);
                if (len >6){
                    if (strncasecmp(line,"!AIVDM",6) == 0
                        ||
                        strncasecmp(line,"!AIVDO",6) == 0
                    ) isAis=true;
                }
            }
            String getKey(){
                if (!isAis) return MessageCode();
                char buf[6];
                strncpy(buf,line+1,5);
                buf[5]=0;
                return String(buf);
            }

    };
class NMEA0183DataToN2KFunctions : public NMEA0183DataToN2K
{
private:   
    MyAisDecoder *aisDecoder=NULL;
    ConverterList<NMEA0183DataToN2KFunctions, SNMEA0183Msg> converters;
    std::map<unsigned long,unsigned long> lastSends;
    class WaypointNumber{
        public:
            unsigned long id;
            unsigned long lastUsed;
            WaypointNumber(){}
            WaypointNumber(unsigned long id,unsigned long ts){
                this->id=id;
                this->lastUsed=ts;
            }
    };
    const size_t MAXWAYPOINTS=100;
    std::map<String,WaypointNumber> waypointMap;
    uint8_t waypointId=1;
    
    uint8_t getWaypointId(const char *name){
        String wpName(name);
        auto it=waypointMap.find(wpName);
        if (it != waypointMap.end()){
            it->second.lastUsed=millis();
            return it->second.id;
        }
        unsigned long now=millis();
        auto oldestIt=waypointMap.begin();
        uint8_t newNumber=0;
        if (waypointMap.size() > MAXWAYPOINTS){
            LOG_DEBUG(GwLog::DEBUG+1,"removing oldest from waypoint map");
            for (it=waypointMap.begin();it!=waypointMap.end();it++){
                if (oldestIt->second.lastUsed > it->second.lastUsed){
                    oldestIt=it;
                }
            }
            newNumber=oldestIt->second.id;
            waypointMap.erase(oldestIt);
        }
        else{
            waypointId++;
            newNumber=waypointId;
        }
        WaypointNumber newWp(newNumber,now);
        waypointMap[wpName]=newWp;
        return newWp.id;
    }
    bool send(tN2kMsg &msg,unsigned long minDiff=50){
        unsigned long now=millis();
        unsigned long pgn=msg.PGN;
        auto it=lastSends.find(pgn);
        if (it == lastSends.end()){
            lastSends[pgn]=now;
            sender(msg);
            return true;
        }
        if ((it->second + minDiff) <= now){
            lastSends[pgn]=now;
            sender(msg);
            return true;
        }
        return false;
    }
    bool updateDouble(GwBoatItem<double> *target,double v, int sourceId){
        if (v != NMEA0183DoubleNA){
            return target->update(v,sourceId);
        }
        return false;
    }
    bool updateUint32(GwBoatItem<uint32_t> *target,uint32_t v, int sourceId){
        if (v != NMEA0183UInt32NA){
            return target->update(v,sourceId);
        }
        return v;
    }
    uint32_t getUint32(GwBoatItem<uint32_t> *src){
        return src->getDataWithDefault(N2kUInt32NA);
    }
    #define UD(item) updateDouble(boatData->item, item, msg.sourceId)
    #define UI(item) updateUint32(boatData->item, item, msg.sourceId)
    void convertRMB(const SNMEA0183Msg &msg)
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "convert RMB");
        tRMB rmb;
        if (! NMEA0183ParseRMB_nc(msg,rmb)){
            logger->logDebug(GwLog::DEBUG, "failed to parse RMC %s", msg.line);
            return;   
        }
        tN2kMsg n2kMsg;
        if (boatData->XTE->update(rmb.xte,msg.sourceId)){
            tN2kXTEMode mode=N2kxtem_Autonomous;
            if (msg.FieldCount() > 13){
                const char *modeChar=msg.Field(13);
                switch(*modeChar){
                    case 'D':
                        mode=N2kxtem_Differential;
                        break;
                    case 'E':
                        mode=N2kxtem_Estimated;
                        break;
                    case 'M':
                        mode=N2kxtem_Manual;
                        break;
                    case 'S':
                        mode=N2kxtem_Simulator;
                        break;
                    default:
                        break;    

                }
            }
            SetN2kXTE(n2kMsg,1,mode,false,rmb.xte);
            send(n2kMsg);
        }
        uint8_t destinationId=getWaypointId(rmb.destID);
        uint8_t sourceId=getWaypointId(rmb.originID);
        if (boatData->DTW->update(rmb.dtw,msg.sourceId)
            && boatData->BTW->update(rmb.btw,msg.sourceId)
            && boatData->WPLatitude->update(rmb.latitude,msg.sourceId)
            && boatData->WPLongitude->update(rmb.longitude,msg.sourceId)
            ){
            SetN2kNavigationInfo(n2kMsg,1,rmb.dtw,N2khr_true,
                false,
                (rmb.arrivalAlarm == 'A'),
                N2kdct_GreatCircle, //see e.g. https://manuals.bandg.com/discontinued/NMEA_FFD_User_Manual.pdf pg 21
                N2kDoubleNA,
                N2kUInt16NA,
                N2kDoubleNA,
                rmb.btw,
                sourceId,
                destinationId,
                rmb.latitude,
                rmb.longitude,
                rmb.vmg
            );
            send(n2kMsg);
            SetN2kPGN129285(n2kMsg,sourceId,1,1,true,true,"default");
            AppendN2kPGN129285(n2kMsg,destinationId,rmb.destID,rmb.latitude,rmb.longitude);
            send(n2kMsg);
            }
    }
    void convertRMC(const SNMEA0183Msg &msg)
    {
        double SecondsSinceMidnight=0, Latitude=0, Longitude=0, COG=0, SOG=0, Variation=0;
        unsigned long DaysSince1970=0;
        time_t DateTime;
        char status;
        if (!NMEA0183ParseRMC_nc(msg, SecondsSinceMidnight, status, Latitude, Longitude, COG, SOG, DaysSince1970, Variation, &DateTime))
        {
            logger->logDebug(GwLog::DEBUG, "failed to parse RMC %s", msg.line);
            return;
        }
        tN2kMsg n2kMsg;
        if (
            UD(SecondsSinceMidnight) &&
            UI(DaysSince1970)
        )
        {
            
            SetN2kSystemTime(n2kMsg, 1, DaysSince1970, SecondsSinceMidnight);
            send(n2kMsg);
        }
        if (UD(Latitude) &&
            UD(Longitude)){
            SetN2kLatLonRapid(n2kMsg,Latitude,Longitude);
            send(n2kMsg);
        }
        if (UD(COG) && UD(SOG)){
            SetN2kCOGSOGRapid(n2kMsg,1,N2khr_true,COG,SOG);
            send(n2kMsg);
        }
        if (UD(Variation)){
            SetN2kMagneticVariation(n2kMsg,1,N2kmagvar_Calc,
                getUint32(boatData->DaysSince1970), Variation);
            send(n2kMsg);    
        }

    }
    void convertAIVDX(const SNMEA0183Msg &msg){
        aisDecoder->handleMessage(msg.line);
    }
    void convertMWV(const SNMEA0183Msg &msg){
        double WindAngle,WindSpeed;
        tNMEA0183WindReference Reference;

        if (!NMEA0183ParseMWV_nc(msg, WindAngle, Reference,WindSpeed))
        {
            logger->logDebug(GwLog::DEBUG, "failed to parse MWV %s", msg.line);
            return;
        }
        tN2kMsg n2kMsg;
        tN2kWindReference n2kRef;
        bool shouldSend=false;
        WindAngle=formatDegToRad(WindAngle);
        switch(Reference){
            case NMEA0183Wind_Apparent:
                n2kRef=N2kWind_Apparent;
                shouldSend=updateDouble(boatData->AWA,WindAngle,msg.sourceId) && 
                    updateDouble(boatData->AWS,WindSpeed,msg.sourceId);
                break;
            case NMEA0183Wind_True:
                n2kRef=N2kWind_True_North;
                shouldSend=updateDouble(boatData->TWD,WindAngle,msg.sourceId) && 
                    updateDouble(boatData->TWS,WindSpeed,msg.sourceId);
                break;      
            default:
                LOG_DEBUG(GwLog::DEBUG,"unknown wind reference %d in %s",(int)Reference,msg.line);
        }
        if (shouldSend){
            SetN2kWindSpeed(n2kMsg,1,WindSpeed,WindAngle,n2kRef);  
            send(n2kMsg);  
        }
    }
    void convertVWR(const SNMEA0183Msg &msg)
    {
        double WindAngle = NMEA0183DoubleNA, WindSpeed = NMEA0183DoubleNA;
        if (msg.FieldCount() < 8 || msg.FieldLen(0) < 1)
        {
            logger->logDebug(GwLog::DEBUG, "failed to parse VWR %s", msg.line);
            return;
        }
        WindAngle = atof(msg.Field(0));
        char direction = msg.Field(1)[0];
        if (direction == 'L' && WindAngle < 180)
            WindAngle = 360 - WindAngle;
        WindAngle = formatDegToRad(WindAngle);
        if (msg.FieldLen(2) > 0 && msg.Field(3)[0] == 'N')
        {
            WindSpeed = atof(msg.Field(2)) * knToms;
        }
        else if (msg.FieldLen(4) > 0 && msg.Field(5)[0] == 'M')
        {
            WindSpeed = atof(msg.Field(4));
        }
        else if (msg.FieldLen(6) > 0 && msg.Field(7)[0] == 'K')
        {
            WindSpeed = atof(msg.Field(6)) * 1000.0 / 3600.0;
        }
        if (WindSpeed == NMEA0183DoubleNA)
        {
            logger->logDebug(GwLog::DEBUG, "no wind speed in VWR %s", msg.line);
            return;
        }
        tN2kMsg n2kMsg;
        bool shouldSend = false;
        shouldSend = updateDouble(boatData->AWA, WindAngle, msg.sourceId) &&
                     updateDouble(boatData->AWS, WindSpeed, msg.sourceId);
        if (shouldSend)
        {
            SetN2kWindSpeed(n2kMsg, 1, WindSpeed, WindAngle, N2kWind_Apparent);
            send(n2kMsg);
        }
    }

    void convertMWD(const SNMEA0183Msg &msg)
    {
        double WindAngle = NMEA0183DoubleNA, WindAngleMagnetic=NMEA0183DoubleNA,
            WindSpeed = NMEA0183DoubleNA;
        if (msg.FieldCount() < 8 )
        {
            logger->logDebug(GwLog::DEBUG, "failed to parse MWD %s", msg.line);
            return;
        }
        if (msg.FieldLen(0) > 0 && msg.Field(1)[0] == 'T')
        {
            WindAngle = formatDegToRad(atof(msg.Field(0)));
        }
        if (msg.FieldLen(2) > 0 && msg.Field(3)[0] == 'M')
        {
            WindAngleMagnetic = formatDegToRad(atof(msg.Field(2)));
        }
        if (msg.FieldLen(4) > 0 && msg.Field(5)[0] == 'N')
        {
            WindSpeed = atof(msg.Field(4)) * knToms;
        }
        else if (msg.FieldLen(6) > 0 && msg.Field(7)[0] == 'M')
        {
            WindSpeed = atof(msg.Field(6));
        }
        if (WindSpeed == NMEA0183DoubleNA)
        {
            logger->logDebug(GwLog::DEBUG, "no wind speed in MWD %s", msg.line);
            return;
        }
        tN2kMsg n2kMsg;
        bool shouldSend = false;
        if (WindAngle != NMEA0183DoubleNA){
            shouldSend = updateDouble(boatData->TWD, WindAngle, msg.sourceId) &&
                         updateDouble(boatData->TWS, WindSpeed, msg.sourceId);
        }
        if (shouldSend)
        {
            SetN2kWindSpeed(n2kMsg, 1, WindSpeed, WindAngle, N2kWind_True_North);
            send(n2kMsg);
        }
        if (WindAngleMagnetic != NMEA0183DoubleNA && shouldSend){
            SetN2kWindSpeed(n2kMsg, 1, WindSpeed, WindAngleMagnetic, N2kWind_Magnetic);
            send(n2kMsg,0); //force sending
        }
    }

//shortcut for lambda converters
#define CVL [](const SNMEA0183Msg &msg, NMEA0183DataToN2KFunctions *p) -> void
    void registerConverters()
    {
        converters.registerConverter(129283UL,129284UL,129285UL,
            String(F("RMB")), &NMEA0183DataToN2KFunctions::convertRMB);
        converters.registerConverter(
            126992UL,129025UL,129026UL,127258UL, 
            String(F("RMC")),  &NMEA0183DataToN2KFunctions::convertRMC);
        converters.registerConverter(
            130306UL,
            String(F("MWV")),&NMEA0183DataToN2KFunctions::convertMWV); 
        converters.registerConverter(
            130306UL,
            String(F("MWD")),&NMEA0183DataToN2KFunctions::convertMWD);     
        converters.registerConverter(
            130306UL,
            String(F("VWR")),&NMEA0183DataToN2KFunctions::convertVWR);        
        unsigned long *aispgns=new unsigned long[7]{129810UL,129809UL,129040UL,129039UL,129802UL,129794UL,129038UL};
        converters.registerConverter(7,&aispgns[0],
            String(F("AIVDM")),&NMEA0183DataToN2KFunctions::convertAIVDX);
        converters.registerConverter(7,&aispgns[0],
            String(F("AIVDO")),&NMEA0183DataToN2KFunctions::convertAIVDX);    
    }

public:
    virtual bool parseAndSend(const char *buffer, int sourceId)
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "NMEA0183DataToN2K[%d] parsing %s", sourceId, buffer)
        SNMEA0183Msg msg(buffer,sourceId);
        if (! msg.isAis){
            if (!msg.SetMessage(buffer))
            {
                LOG_DEBUG(GwLog::DEBUG, "NMEA0183DataToN2K[%d] invalid message %s", sourceId, buffer)
                return false;
            }
        }
        String code = msg.getKey();
        bool rt = converters.handleMessage(code, msg, this);
        if (!rt)
        {
            LOG_DEBUG(GwLog::DEBUG, "NMEA0183DataToN2K[%d] no handler for (%s) %s", sourceId, code.c_str(), buffer);
        }
        else{
            LOG_DEBUG(GwLog::DEBUG+1, "NMEA0183DataToN2K[%d] handler done ", sourceId);
        }
        return rt;
    }

    virtual unsigned long *handledPgns()
    {
        return converters.handledPgns();
    }

    virtual int numConverters(){
        return converters.numConverters();
    }

    NMEA0183DataToN2KFunctions(GwLog *logger, GwBoatData *boatData, N2kSender callback)
        : NMEA0183DataToN2K(logger, boatData, callback)
    {
        aisDecoder= new MyAisDecoder(logger,this->sender);
        registerConverters();
        LOG_DEBUG(GwLog::LOG, "NMEA0183DataToN2KFunctions: registered %d converters", converters.numConverters());
    }
    };

NMEA0183DataToN2K* NMEA0183DataToN2K::create(GwLog *logger,GwBoatData *boatData,N2kSender callback){
    return new NMEA0183DataToN2KFunctions(logger, boatData,callback);

}
