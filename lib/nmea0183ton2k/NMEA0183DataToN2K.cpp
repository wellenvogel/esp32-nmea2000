#include "NMEA0183DataToN2K.h"
#include "NMEA0183Messages.h"
#include "N2kMessages.h"
#include "ConverterList.h"
#include <map>
#include <strings.h>
#include "NMEA0183AIStoNMEA2000.h"

static const double mToFathoms=0.546806649;
static const double mToFeet=3.2808398950131;

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
            LOG_DEBUG(GwLog::DEBUG, "failed to parse RMC %s", msg.line);
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
            LOG_DEBUG(GwLog::DEBUG, "failed to parse RMC %s", msg.line);
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
            LOG_DEBUG(GwLog::DEBUG, "failed to parse MWV %s", msg.line);
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
                if (WindSpeed != NMEA0183DoubleNA) boatData->MaxAws->updateMax(WindSpeed);    
                break;
            case NMEA0183Wind_True:
                n2kRef=N2kWind_True_North;
                shouldSend=updateDouble(boatData->TWD,WindAngle,msg.sourceId) && 
                    updateDouble(boatData->TWS,WindSpeed,msg.sourceId);
                if (WindSpeed != NMEA0183DoubleNA) boatData->MaxTws->updateMax(WindSpeed);    
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
            LOG_DEBUG(GwLog::DEBUG, "failed to parse VWR %s", msg.line);
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
        if (WindSpeed != NMEA0183DoubleNA) boatData->MaxAws->updateMax(WindSpeed);             
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
            LOG_DEBUG(GwLog::DEBUG, "failed to parse MWD %s", msg.line);
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
            if (WindSpeed != NMEA0183DoubleNA) boatData->MaxTws->updateMax(WindSpeed);             
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

    void convertHDM(const SNMEA0183Msg &msg){
        double MagneticHeading=NMEA0183DoubleNA;
        if (!NMEA0183ParseHDM_nc(msg, MagneticHeading))
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse HDM %s", msg.line);
            return;
        }
        if (! UD(MagneticHeading)) return;
        tN2kMsg n2kMsg;
        SetN2kMagneticHeading(n2kMsg,1,MagneticHeading,
            boatData->Variation->getDataWithDefault(N2kDoubleNA),
            boatData->Deviation->getDataWithDefault(N2kDoubleNA)
        );
        send(n2kMsg);    
    }
    
    void convertHDT(const SNMEA0183Msg &msg){
        double Heading=NMEA0183DoubleNA;
        if (!NMEA0183ParseHDT_nc(msg, Heading))
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse HDT %s", msg.line);
            return;
        }
        if (! UD(Heading)) return;
        tN2kMsg n2kMsg;
        SetN2kTrueHeading(n2kMsg,1,Heading);
        send(n2kMsg);    
    }
    void convertHDG(const SNMEA0183Msg &msg){
        double MagneticHeading=NMEA0183DoubleNA;
        double Variation=NMEA0183DoubleNA;
        double Deviation=NMEA0183DoubleNA;
        if (msg.FieldCount() < 5)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse HDG %s", msg.line);
            return;
        }
        if (msg.FieldLen(0)>0){
            MagneticHeading=formatDegToRad(atof(msg.Field(0)));
        }
        else{
            return;
        }
        if (msg.FieldLen(1)>0){
            Deviation=formatDegToRad(atof(msg.Field(1)));
            if (msg.Field(2)[0] == 'W') Deviation=-Deviation;
        }
        if (msg.FieldLen(3)>0){
            Variation=formatDegToRad(atof(msg.Field(3)));
            if (msg.Field(4)[0] == 'W') Variation=-Variation;
        }

        if (! UD(MagneticHeading)) return;
        UD(Variation);
        UD(Deviation);
        tN2kMsg n2kMsg;
        SetN2kMagneticHeading(n2kMsg,1,MagneticHeading,Deviation,Variation);
        send(n2kMsg);    
    }

    void convertDPT(const SNMEA0183Msg &msg){
        double DepthBelowTransducer=NMEA0183DoubleNA;
        double Offset=NMEA0183DoubleNA;
        if (msg.FieldCount() < 2)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse DPT %s", msg.line);
            return;
        }
        if (msg.FieldLen(0)>0){
            DepthBelowTransducer=atof(msg.Field(0));
        }
        else{
            return;
        }
        if (msg.FieldLen(1)>0){
            Offset=atof(msg.Field(1));
        }
        //offset == 0? SK does not allow this
        if (Offset != NMEA0183DoubleNA && Offset>=0 ){
            if (! boatData->WaterDepth->update(DepthBelowTransducer+Offset)) return;
        }
        if (Offset == NMEA0183DoubleNA) Offset=N2kDoubleNA;
        if (! boatData->DepthTransducer->update(DepthBelowTransducer)) return;
        tN2kMsg n2kMsg;
        SetN2kWaterDepth(n2kMsg,1,DepthBelowTransducer,Offset);
        send(n2kMsg);
    }
    typedef enum {
        DBS,
        DBK,
        DBT
    } DepthType;
    void convertDBKx(const SNMEA0183Msg &msg,DepthType dt){
        double Depth=NMEA0183DoubleNA;
        if (msg.FieldCount() < 6)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse DBK/DBS %s", msg.line);
            return;
        }
        for (int i=0;i< 3;i++){
            if (msg.FieldLen(0)>0){
                Depth=atof(msg.Field(0));
                char dt=msg.Field(i+1)[0];
                switch(dt){
                    case 'f':
                        Depth=Depth/mToFeet;
                        break;
                    case 'M':
                        break;
                    case 'F':
                        Depth=Depth/mToFathoms;
                        break;
                    default:
                        //unknown type, try next
                        continue;            
                }
                if (dt == DBT){
                    if (! boatData->DepthTransducer->update(Depth,msg.sourceId)) return;
                    tN2kMsg n2kMsg;
                    SetN2kWaterDepth(n2kMsg,1,Depth,N2kDoubleNA); 
                    send(n2kMsg);
                    return;   
                }
                //we can only send if we have a valid depth beloww tranducer
                //to compute the offset
                if (! boatData->DepthTransducer->isValid()) return;
                double offset=Depth-boatData->DepthTransducer->getData();
                if (offset >= 0 && dt == DBT){
                    logger->logDebug(GwLog::DEBUG, "strange DBK - more depth then transducer %s", msg.line);    
                    return;
                }
                if (offset < 0 && dt == DBS){
                    logger->logDebug(GwLog::DEBUG, "strange DBS - less depth then transducer %s", msg.line);    
                    return;
                }
                if (dt == DBS){
                    if (! boatData->WaterDepth->update(Depth,msg.sourceId)) return;
                }
                tN2kMsg n2kMsg;
                SetN2kWaterDepth(n2kMsg,1,Depth,offset);
                send(n2kMsg);
            }            
        }        
    }
    void convertDBK(const SNMEA0183Msg &msg){
        return convertDBKx(msg,DBK);
    }
    void convertDBS(const SNMEA0183Msg &msg){
        return convertDBKx(msg,DBS);
    }
    void convertDBT(const SNMEA0183Msg &msg){
        return convertDBKx(msg,DBT);
    }

    void convertRSA(const SNMEA0183Msg &msg){
        double RudderPosition=NMEA0183DoubleNA;
        if (msg.FieldCount() < 4)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse RSA %s", msg.line);
            return;
        }
        if (msg.FieldLen(0)>0){
            if (msg.Field(1)[0] != 'A') return;
            RudderPosition=degToRad*atof(msg.Field(0));
            tN2kMsg n2kMsg;
            if (! UD(RudderPosition)) return;
            SetN2kRudder(n2kMsg,RudderPosition);
            send(n2kMsg);
        }
          
    }
    void convertVHW(const SNMEA0183Msg &msg){
        double TrueHeading=NMEA0183DoubleNA;
        double MagneticHeading=NMEA0183DoubleNA;
        double STW=NMEA0183DoubleNA;
        if (! NMEA0183ParseVHW_nc(msg,TrueHeading,MagneticHeading,STW)){
            LOG_DEBUG(GwLog::DEBUG, "failed to parse VHW %s", msg.line);
            return;
        }
        if (! updateDouble(boatData->STW,STW,msg.sourceId)) return;
        if (! updateDouble(boatData->Heading,TrueHeading,msg.sourceId)) return;
        if (MagneticHeading == NMEA0183DoubleNA) MagneticHeading=N2kDoubleNA;
        tN2kMsg n2kMsg;
        SetN2kBoatSpeed(n2kMsg,1,STW);
        send(n2kMsg);

    }
    void convertVTG(const SNMEA0183Msg &msg){
        double COG=NMEA0183DoubleNA;
        double SOG=NMEA0183DoubleNA;
        double MCOG=NMEA0183DoubleNA;
        if (! NMEA0183ParseVTG_nc(msg,COG,MCOG,SOG)){
            LOG_DEBUG(GwLog::DEBUG, "failed to parse VTG %s", msg.line);
            return;   
        }
        if (! UD(COG)) return;
        if (! UD(SOG)) return;
        tN2kMsg n2kMsg;
        //TODO: maybe use MCOG if no COG?
        SetN2kCOGSOGRapid(n2kMsg,1,N2khr_true,COG,SOG);
        send(n2kMsg);
    }
    void convertZDA(const SNMEA0183Msg &msg){
        time_t DateTime;
        long Timezone;
        if (! NMEA0183ParseZDA(msg,DateTime,Timezone)){
            LOG_DEBUG(GwLog::DEBUG, "failed to parse ZDA %s", msg.line);
            return;   
        }
        uint32_t DaysSince1970=tNMEA0183Msg::elapsedDaysSince1970(DateTime);
        tmElements_t parts;
        tNMEA0183Msg::breakTime(DateTime,parts);
        double SecondsSinceMidnight=parts.tm_sec+60*parts.tm_min+3600*parts.tm_hour;
        if (! boatData->DaysSince1970->update(DaysSince1970,msg.sourceId)) return;
        if (! boatData->SecondsSinceMidnight->update(SecondsSinceMidnight,msg.sourceId)) return;
        bool timezoneValid=false;
        if (msg.FieldLen(4) > 0 && msg.FieldLen(5)>0){
            Timezone=Timezone/60; //N2K has offset in minutes
            if (! boatData->Timezone->update(Timezone,msg.sourceId)) return;
            timezoneValid=true;
        }
        tN2kMsg n2kMsg;
        if (timezoneValid){
            SetN2kLocalOffset(n2kMsg,DaysSince1970,SecondsSinceMidnight,Timezone);
            send(n2kMsg);
        }
        SetN2kSystemTime(n2kMsg,1,DaysSince1970,SecondsSinceMidnight);
        send(n2kMsg);
    }
    void convertGGA(const SNMEA0183Msg &msg){
        double GPSTime=NMEA0183DoubleNA;
        double Latitude=NMEA0183DoubleNA;
        double Longitude=NMEA0183DoubleNA;
        int GPSQualityIndicator=NMEA0183Int32NA;
        int SatelliteCount=NMEA0183Int32NA;
        double HDOP=NMEA0183DoubleNA; 
        double Altitude=NMEA0183DoubleNA;
        double GeoidalSeparation=NMEA0183DoubleNA;
        double DGPSAge=NMEA0183DoubleNA;
        int DGPSReferenceStationID=NMEA0183Int32NA;
        if (! NMEA0183ParseGGA_nc(msg,GPSTime, Latitude,Longitude,
                      GPSQualityIndicator, SatelliteCount, HDOP, Altitude,GeoidalSeparation,
                      DGPSAge, DGPSReferenceStationID)){
            LOG_DEBUG(GwLog::DEBUG, "failed to parse GGA %s", msg.line);
            return;   
        }
        if (! updateDouble(boatData->SecondsSinceMidnight,GPSTime,msg.sourceId)) return;
        if (! updateDouble(boatData->Latitude,Latitude,msg.sourceId)) return;        
        if (! updateDouble(boatData->Longitude,Longitude,msg.sourceId)) return;    
        if (! updateDouble(boatData->Altitude,Altitude,msg.sourceId)) return;
        if (! updateDouble(boatData->HDOP,HDOP,msg.sourceId)) return;    
        if (! boatData->DaysSince1970->isValid()) return;
        tN2kMsg n2kMsg;
        tN2kGNSSmethod method=N2kGNSSm_noGNSS;
        if (GPSQualityIndicator <=5 ) method= (tN2kGNSSmethod)GPSQualityIndicator;
        SetN2kGNSS(n2kMsg,1, boatData->DaysSince1970->getData(),
             GPSTime, Latitude, Longitude, Altitude,
                     N2kGNSSt_GPS, method,
                     SatelliteCount, HDOP, boatData->PDOP->getDataWithDefault(N2kDoubleNA), 0,
                     0, N2kGNSSt_GPS, DGPSReferenceStationID,
                     DGPSAge);
        send(n2kMsg);             
    }
    void convertGSA(const SNMEA0183Msg &msg){
        if (msg.FieldCount() < 17)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse GSA %s", msg.line);
            return;
        }
        int fixMode=atoi(msg.Field(1));

        tN2kMsg n2kMsg;
        tN2kGNSSDOPmode mode=N2kGNSSdm_Unavailable;
        if (fixMode >= 0 && fixMode <=3) mode=(tN2kGNSSDOPmode)fixMode;
        tN2kGNSSDOPmode rmode=mode;
        if (msg.Field(0)[0] == 'A') rmode=N2kGNSSdm_Auto;
        double HDOP=N2kDoubleNA;
        double VDOP=N2kDoubleNA;
        double PDOP=N2kDoubleNA;
        if (msg.FieldLen(14)> 0){
            PDOP=atof(msg.Field(14));
            if (!updateDouble(boatData->PDOP,PDOP,msg.sourceId)) return;
        }
        if (msg.FieldLen(15)> 0){
            HDOP=atof(msg.Field(15));
            if (!updateDouble(boatData->HDOP,HDOP,msg.sourceId)) return;
        }
        if (msg.FieldLen(16)> 0){
            VDOP=atof(msg.Field(16));
            if (!updateDouble(boatData->VDOP,VDOP,msg.sourceId)) return;
        }
        SetN2kGNSSDOPData(n2kMsg,1,rmode,mode,HDOP,VDOP,N2kDoubleNA);
        send(n2kMsg);
    }
    void convertGSV(const SNMEA0183Msg &msg){
        if (msg.FieldCount() < 7){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse GSV %s",msg.line);
            return;
        }
        uint32_t total=atoi(msg.Field(0));
        if (total < 1 || total > 9) {
            LOG_DEBUG(GwLog::DEBUG,"GSV invalid total %u %s",total,msg.line);
            return;
        }
        uint32_t current=atoi(msg.Field(1));
        if (current < 1 || current > total) {
            LOG_DEBUG(GwLog::DEBUG,"GSV invalid current %u %s",current,msg.line);
            return;
        }
        for (int idx=2;idx < msg.FieldCount();idx+=4){
            if (msg.FieldLen(idx) < 1 ||
                msg.FieldLen(idx+1) < 1 ||
                msg.FieldLen(idx+2) < 1 ||
                msg.FieldLen(idx+3) < 1
            ) continue;
            GwSatInfo info;
            info.PRN=atoi(msg.Field(idx));
            info.Elevation=atoi(msg.Field(idx+1));
            info.Azimut=atoi(msg.Field(idx+2));
            info.SNR=atoi(msg.Field(idx+3));
            if (!boatData->SatInfo->update(info,msg.sourceId)) return;
        }
        int numSat=boatData->SatInfo->getNumSats();
        if (current == total && numSat > 0){
            tN2kMsg n2kMsg;
            SetN2kGNSSSatellitesInView(n2kMsg,1);
            bool hasInfos=false;
            for (int i=0;i<numSat;i++){
                tSatelliteInfo satInfo;
                GwSatInfo *gwInfo=boatData->SatInfo->getAt(i);
                if (gwInfo){
                    hasInfos=true;
                    satInfo.PRN=gwInfo->PRN;
                    satInfo.SNR=gwInfo->SNR;
                    satInfo.Elevation=DegToRad(gwInfo->Elevation);
                    satInfo.Azimuth=DegToRad(gwInfo->Azimut);
                    AppendSatelliteInfo(n2kMsg,satInfo);
                }
            }
            if (hasInfos){
                send(n2kMsg);
            }

        }
    }

    void convertGLL(const SNMEA0183Msg &msg){
        tGLL GLL;
        if (! NMEA0183ParseGLL_nc(msg,GLL)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse GLL %s",msg.line);
            return;
        }
        if (GLL.status != 'A') return;
        if (! updateDouble(boatData->Latitude,GLL.latitude,msg.sourceId)) return;
        if (! updateDouble(boatData->Longitude,GLL.longitude,msg.sourceId)) return;
        if (! updateDouble(boatData->SecondsSinceMidnight,GLL.GPSTime,msg.sourceId)) return;
        tN2kMsg n2kMsg;
        SetN2kLatLonRapid(n2kMsg,GLL.latitude,GLL.longitude);
        send(n2kMsg);
    }

    void convertROT(const SNMEA0183Msg &msg){
        double ROT=NMEA0183DoubleNA;
        if (! NMEA0183ParseROT_nc(msg,ROT)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse ROT %s",msg.line);
            return;
        }
        if (! updateDouble(boatData->ROT,ROT,msg.sourceId)) return;
        tN2kMsg n2kMsg;
        SetN2kRateOfTurn(n2kMsg,1,ROT);
        send(n2kMsg);
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
        converters.registerConverter(
            127250UL,
            String(F("HDM")),&NMEA0183DataToN2KFunctions::convertHDM); 
        converters.registerConverter(
            127250UL,
            String(F("HDT")),&NMEA0183DataToN2KFunctions::convertHDT);
        converters.registerConverter(
            127250UL,
            String(F("HDG")),&NMEA0183DataToN2KFunctions::convertHDG);
        converters.registerConverter(
            128267UL,
            String(F("DPT")), &NMEA0183DataToN2KFunctions::convertDPT);
        converters.registerConverter(
            128267UL,
            String(F("DBK")), &NMEA0183DataToN2KFunctions::convertDBK);    
        converters.registerConverter(
            128267UL,
            String(F("DBS")), &NMEA0183DataToN2KFunctions::convertDBS);
        converters.registerConverter(
            128267UL,
            String(F("DBT")), &NMEA0183DataToN2KFunctions::convertDBT);
        converters.registerConverter(
            127245UL,
            String(F("RSA")), &NMEA0183DataToN2KFunctions::convertRSA); 
        converters.registerConverter(
            128259UL,
            String(F("VHW")), &NMEA0183DataToN2KFunctions::convertVHW);
        converters.registerConverter(
            129026UL,
            String(F("VTG")), &NMEA0183DataToN2KFunctions::convertVTG);
        converters.registerConverter(
            129033UL,126992UL,
            String(F("ZDA")), &NMEA0183DataToN2KFunctions::convertZDA);
        converters.registerConverter(
            129029UL,
            String(F("GGA")), &NMEA0183DataToN2KFunctions::convertGGA); 
        converters.registerConverter(
            129539UL,
            String(F("GSA")), &NMEA0183DataToN2KFunctions::convertGSA); 
        converters.registerConverter(
            129540UL,
            String(F("GSV")), &NMEA0183DataToN2KFunctions::convertGSV);
        converters.registerConverter(
            129025UL,
            String(F("GLL")), &NMEA0183DataToN2KFunctions::convertGLL); 
        converters.registerConverter(
            127251UL,
            String(F("ROT")), &NMEA0183DataToN2KFunctions::convertROT);     
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
    virtual String handledKeys(){
        return converters.handledKeys();
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
