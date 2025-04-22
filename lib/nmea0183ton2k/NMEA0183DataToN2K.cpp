#include "NMEA0183DataToN2K.h"
#include "NMEA0183Messages.h"
#include "N2kMessages.h"
#include "ConverterList.h"
#include <map>
#include <set>
#include <strings.h>
#include "NMEA0183AIStoNMEA2000.h"
#include "GwXDRMappings.h"
#include "GwNmea0183Msg.h"

static const double mToFathoms=0.546806649;
static const double mToFeet=3.2808398950131;

NMEA0183DataToN2K::NMEA0183DataToN2K(GwLog *logger, GwBoatData *boatData,N2kSender callback)
{
    this->sender = callback;
    this->logger = logger;
    this->boatData=boatData;
    LOG_DEBUG(GwLog::LOG,"NMEA0183DataToN2K created %p",this);
}




class NMEA0183DataToN2KFunctions : public NMEA0183DataToN2K
{
private:   
    MyAisDecoder *aisDecoder=NULL;
    ConverterList<NMEA0183DataToN2KFunctions, SNMEA0183Msg> converters;
    std::map<String,unsigned long> lastSends;
    GwXDRMappings *xdrMappings;
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
    bool send(tN2kMsg &msg,String key,unsigned long minDiff,int sourceId){
        unsigned long now=millis();
        unsigned long pgn=msg.PGN;
        if (key == "") key=String(msg.PGN);
        auto it=lastSends.find(key);
        if (it == lastSends.end()){
            lastSends[key]=now;
            sender(msg,sourceId);
            return true;
        }
        if ((it->second + minDiff) <= now){
            lastSends[key]=now;
            sender(msg,sourceId);
            return true;
        }
        LOG_DEBUG(GwLog::DEBUG+1,"skipped n2k message %d",msg.PGN);
        return false;
    }
    bool send(tN2kMsg &msg, int sourceId,String key=""){
        return send(msg,key,config.min2KInterval,sourceId);
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
        return false;
    }
    uint32_t getUint32(GwBoatItem<uint32_t> *src){
        return src->getDataWithDefault(N2kUInt32NA);
    }
    #define UD(item) updateDouble(boatData->item, item, msg.sourceId)
    #define UI(item) updateUint32(boatData->item, item, msg.sourceId)
    tN2kXTEMode xteMode(const char modeChar){
        switch(modeChar){
                    case 'D':
                        return N2kxtem_Differential;
                    case 'E':
                        return N2kxtem_Estimated;
                    case 'M':
                        return N2kxtem_Manual;
                    case 'S':
                        return N2kxtem_Simulator;
                    default:
                        break; 
        }
        return N2kxtem_Autonomous;     
    }
    class XdrMappingAndValue{
        public:
            GwXDRFoundMapping mapping;
            double value;
            XdrMappingAndValue(GwXDRFoundMapping &mapping,double value){
                this->mapping=mapping;
                this->value=value;
            }
            int field(){return mapping.definition->field;}
            int selector(){return mapping.definition->selector;}
    };
    typedef std::vector<XdrMappingAndValue> XdrMappingList;
    /**
     * find a mapping for a different field from the same
     * category with similar instanceId and selector
     */
    GwXDRFoundMapping getOtherFieldMapping(GwXDRFoundMapping &found, int field){
        if (found.empty) return GwXDRFoundMapping();
        return xdrMappings->getMapping(found.definition->category,
            found.definition->selector,
            field,
            found.instanceId);
    }
    double getOtherFieldValue(GwXDRFoundMapping &found, int field){
        GwXDRFoundMapping other=getOtherFieldMapping(found,field);
        if (other.empty) return N2kDoubleNA;
        LOG_DEBUG(GwLog::DEBUG+1,"found other field mapping %s",other.definition->toString().c_str());
        return boatData->getDataWithDefault(N2kDoubleNA,&other);
    }
    /**
     * fill all the fields we potentially need for the n2k message
     * we take the current transducer value from the XdrMappingAndValue and
     * try to find a mapping with similar category/instance/selector but different
     * field id for the other fields
     * if such a mapping exists we try to fetch the entry from boatData
     * if we at least inserted one valid value into the field list
     * we return true so that we can send out the message
     */
    bool fillFieldList(XdrMappingAndValue &current,double *list,int numFields,int start=0){
        bool rt=false;
        for (int i=start;i<numFields;i++){
            if (i == current.field()) *(list+i)=current.value;
            *(list+i)=getOtherFieldValue(current.mapping,i);
            if (! N2kIsNA(*(list+i))) rt=true;
        }
        LOG_DEBUG(GwLog::DEBUG+1,"fillFieldList current=%s, start=%d, num=%d, val=%f, return=%s",
            current.mapping.definition->toString().c_str(),
            start,numFields,
            current.value,
            rt?"true":"false");
        return rt;
    }
    String buildN2KKey(const tN2kMsg &msg,GwXDRFoundMapping &found){
        String rt(msg.PGN);
        rt+=".";
        rt+=String(found.definition->selector);
        rt+=".";
        rt+=String(found.instanceId);
        return rt;
    }
    int8_t fromDouble(double v){
        if (N2kIsNA(v)) return N2kInt8NA;
        return v;
    }
    
    void convertXDR(const SNMEA0183Msg &msg){
        XdrMappingList foundMappings;
        for (int offset=0;offset <= (msg.FieldCount()-4);offset+=4){
            //parse next transducer
            String type=msg.Field(offset);
            if (msg.FieldLen(offset+1) < 1) continue; //empty value
            double value=atof(msg.Field(offset+1));
            String unit=msg.Field(offset+2);
            String transducerName=msg.Field(offset+3);
            GwXDRFoundMapping found=xdrMappings->getMapping(transducerName,type,unit);
            if (found.empty) {
                if (config.unmappedXdr){
                    const GwXDRType *typeDef=xdrMappings->findType(type,unit);
                    GwXdrUnknownMapping mapping(transducerName,unit,typeDef,config.xdrTimeout);
                    value=mapping.valueFromXdr(value);
                    if (boatData->update(value,msg.sourceId,&mapping)){
                        //TODO: potentially update the format
                        LOG_DEBUG(GwLog::DEBUG+1,"found unmapped XDR %s:%s, value %f",
                        transducerName.c_str(),mapping.getBoatItemFormat().c_str(),value);
                    }
                }
                continue;
            }
            value=found.valueFromXdr(value);
            if (!boatData->update(value,msg.sourceId,&found)) continue;
            LOG_DEBUG(GwLog::DEBUG+1,"found mapped XDR %s:%s, value %f",
                transducerName.c_str(),
                found.definition->toString().c_str(),
                value);
            foundMappings.push_back(XdrMappingAndValue(found,value));
        }
        static const int maxFields=20;
        double fields[maxFields];
        //currently we will build similar n2k messages if there are
        //multiple transducers in one XDR
        //but we finally rely on the send interval filter
        //to not send them multiple times
        //it could be optimized by checking if we already created
        //a message with similar key in this round and skipp the fillFieldList
        //in this case
        for (auto it=foundMappings.begin();it != foundMappings.end();it++){
            XdrMappingAndValue current=*it;
            tN2kMsg n2kMsg;
            switch (current.mapping.definition->category)
            {

            case XDRFLUID:
                if (fillFieldList(current, fields, 2))
                {
                    SetN2kPGN127505(n2kMsg, current.mapping.instanceId,
                                    (tN2kFluidType)(current.selector()),
                                    fields[0],
                                    fields[1]);
                    send(n2kMsg,msg.sourceId, buildN2KKey(n2kMsg, current.mapping));
                }
                break;
            case XDRBAT:
                if (fillFieldList(current, fields, 3))
                {
                    SetN2kPGN127508(n2kMsg, current.mapping.instanceId,
                                    fields[0], fields[1], fields[2]);
                    send(n2kMsg,msg.sourceId, buildN2KKey(n2kMsg, current.mapping));
                }
                break;
            case XDRTEMP:
                if (fillFieldList(current,fields,2)){
                    SetN2kPGN130312(n2kMsg,1,current.mapping.instanceId,
                        (tN2kTempSource)(current.selector()),
                        fields[0],fields[1]);
                    send(n2kMsg,msg.sourceId,buildN2KKey(n2kMsg,current.mapping));    
                }
                break;
            case XDRHUMIDITY:
                if (fillFieldList(current,fields,2)){
                    SetN2kPGN130313(n2kMsg,1,current.mapping.instanceId,
                    (tN2kHumiditySource)(current.selector()),
                    fields[0],
                    fields[1]
                    );
                    send(n2kMsg,msg.sourceId,buildN2KKey(n2kMsg,current.mapping));
                }
                break;
            case XDRPRESSURE:
                if (fillFieldList(current,fields,1)){
                    SetN2kPGN130314(n2kMsg,1,current.mapping.instanceId,
                    (tN2kPressureSource)(current.selector()),
                    fields[0]);
                    send(n2kMsg,msg.sourceId,buildN2KKey(n2kMsg,current.mapping));
                }
                break;
            case XDRENGINE:
                if (current.field() <= 9)
                {
                    if (fillFieldList(current, fields, 10))
                    {
                        SetN2kPGN127489(n2kMsg, current.mapping.instanceId,
                                        fields[0], fields[1], fields[2], fields[3], fields[4],
                                        fields[5], fields[6], fields[7], fromDouble(fields[8]), fromDouble(fields[9]),
                                        tN2kEngineDiscreteStatus1(), tN2kEngineDiscreteStatus2());
                        send(n2kMsg,msg.sourceId, buildN2KKey(n2kMsg, current.mapping));
                    }
                }
                else{
                    if (fillFieldList(current, fields, 13,10)){
                        SetN2kPGN127488(n2kMsg,current.mapping.instanceId,
                        fields[10],fields[11],fromDouble(fields[12]));
                        send(n2kMsg,msg.sourceId, buildN2KKey(n2kMsg, current.mapping));
                    }
                }
                break;
            case XDRATTITUDE:
                if (fillFieldList(current,fields,3)){
                    SetN2kPGN127257(n2kMsg,current.mapping.instanceId,fields[0],fields[1],fields[2]);
                    send(n2kMsg,msg.sourceId,buildN2KKey(n2kMsg,current.mapping));
                }    
            default:
                continue;
            }
        }
    }

    void convertRMB(const SNMEA0183Msg &msg)
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "convert RMB");
        tRMB rmb;
        if (! NMEA0183ParseRMB_nc(msg,rmb)){
            LOG_DEBUG(GwLog::DEBUG, "failed to parse RMB %s", msg.line);
            return;   
        }
        tN2kMsg n2kMsg;
        if (updateDouble(boatData->XTE,rmb.xte,msg.sourceId)){
            tN2kXTEMode mode=N2kxtem_Autonomous;
            if (msg.FieldCount() > 13){
                const char *modeChar=msg.Field(13);
                mode=xteMode(*modeChar);
            }
            SetN2kXTE(n2kMsg,1,mode,false,rmb.xte);
            send(n2kMsg,msg.sourceId);
        }
        uint8_t destinationId=getWaypointId(rmb.destID);
        uint8_t sourceId=getWaypointId(rmb.originID);
        if (updateDouble(boatData->DTW,rmb.dtw,msg.sourceId)
            && updateDouble(boatData->BTW,rmb.btw,msg.sourceId)
            && updateDouble(boatData->WPLat,rmb.latitude,msg.sourceId)
            && updateDouble(boatData->WPLon,rmb.longitude,msg.sourceId)
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
            send(n2kMsg,msg.sourceId);
            SetN2kPGN129285(n2kMsg,sourceId,1,1,true,true,"default");
            AppendN2kPGN129285(n2kMsg,destinationId,rmb.destID,rmb.latitude,rmb.longitude);
            send(n2kMsg,msg.sourceId);
            }
    }
    void convertRMC(const SNMEA0183Msg &msg)
    {
        double GPST=0, LAT=0, LON=0, COG=0, SOG=0, VAR=0;
        unsigned long GPSD=0;
        time_t DateTime;
        char status;
        if (!NMEA0183ParseRMC_nc(msg, GPST, status, LAT, LON, COG, SOG, GPSD, VAR, &DateTime))
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse RMC %s", msg.line);
            return;
        }
        if (status != 'A' && status != 'a'){
            LOG_DEBUG(GwLog::DEBUG, "invalid status %c for RMC %s",status, msg.line);
            return;
        }
        lastRmc=millis(); //we received an RMC that is not from us
        tN2kMsg n2kMsg;
        if (
            UD(GPST) &&
            UI(GPSD)
        )
        {
            
            SetN2kSystemTime(n2kMsg, 1, GPSD, GPST);
            send(n2kMsg,msg.sourceId);
        }
        if (UD(LAT) &&
            UD(LON)){
            SetN2kLatLonRapid(n2kMsg,LAT,LON);
            send(n2kMsg,msg.sourceId);
        }
        if (UD(COG) && UD(SOG)){
            SetN2kCOGSOGRapid(n2kMsg,1,N2khr_true,COG,SOG);
            send(n2kMsg,msg.sourceId);
        }
        if (UD(VAR)){
            SetN2kMagneticVariation(n2kMsg,1,N2kmagvar_Calc,
                getUint32(boatData->GPSD), VAR);
            send(n2kMsg,msg.sourceId);    
        }

    }
    void convertAIVDX(const SNMEA0183Msg &msg){
        aisDecoder->sourceId=msg.sourceId;
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
        bool shouldSend=false;
        WindAngle=formatDegToRad(WindAngle);
        GwConverterConfig::WindMapping mapping;
        switch(Reference){
            case NMEA0183Wind_Apparent:
                shouldSend=updateDouble(boatData->AWA,WindAngle,msg.sourceId) && 
                           updateDouble(boatData->AWS,WindSpeed,msg.sourceId);
                if (WindSpeed != NMEA0183DoubleNA) boatData->MaxAws->updateMax(WindSpeed,msg.sourceId);    
                mapping=config.findWindMapping(GwConverterConfig::WindMapping::AWA_AWS);
                break;
            case NMEA0183Wind_True:
                shouldSend=updateDouble(boatData->TWA,WindAngle,msg.sourceId) && 
                           updateDouble(boatData->TWS,WindSpeed,msg.sourceId);
                if (WindSpeed != NMEA0183DoubleNA) boatData->MaxTws->updateMax(WindSpeed,msg.sourceId);    
                mapping=config.findWindMapping(GwConverterConfig::WindMapping::TWA_TWS);
                break;      
            default:
                LOG_DEBUG(GwLog::DEBUG,"unknown wind reference %d in %s",(int)Reference,msg.line);
        }
        //TODO: try to compute TWD and get mapping for this one
        if (shouldSend && mapping.valid){
            SetN2kWindSpeed(n2kMsg,1,WindSpeed,WindAngle,mapping.n2kType);  
            send(n2kMsg,msg.sourceId,String(n2kMsg.PGN)+String((int)mapping.n2kType));  
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
        if (WindSpeed != NMEA0183DoubleNA) boatData->MaxAws->updateMax(WindSpeed,msg.sourceId);             
        if (shouldSend)
        {
            const GwConverterConfig::WindMapping mapping=config.findWindMapping(GwConverterConfig::WindMapping::AWA_AWS);
            if (mapping.valid){
                SetN2kWindSpeed(n2kMsg, 1, WindSpeed, WindAngle, mapping.n2kType);
                send(n2kMsg,msg.sourceId,String(n2kMsg.PGN)+String((int)mapping.n2kType));
            }
        }
    }

    void convertMWD(const SNMEA0183Msg &msg)
    {
        double WindDirection = NMEA0183DoubleNA, WindDirectionMagnetic=NMEA0183DoubleNA, WindSpeed = NMEA0183DoubleNA;
        if (msg.FieldCount() < 8 )
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse MWD %s", msg.line);
            return;
        }
        if (msg.FieldLen(0) > 0 && msg.Field(1)[0] == 'T')
        {
            WindDirection = formatDegToRad(atof(msg.Field(0)));
        }
        if (msg.FieldLen(2) > 0 && msg.Field(3)[0] == 'M')
        {
            WindDirectionMagnetic = formatDegToRad(atof(msg.Field(2)));
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
        if (WindDirection != NMEA0183DoubleNA){
            shouldSend = updateDouble(boatData->TWD, WindDirection, msg.sourceId) &&
                         updateDouble(boatData->TWS, WindSpeed, msg.sourceId);
            if (WindSpeed != NMEA0183DoubleNA) boatData->MaxTws->updateMax(WindSpeed,msg.sourceId);             
            if(shouldSend && boatData->HDT->isValid()) {
                double twa = WindDirection-boatData->HDT->getData();
                if(twa<0) { twa+=2*M_PI; }
                updateDouble(boatData->TWA, twa, msg.sourceId);
                const GwConverterConfig::WindMapping mapping=config.findWindMapping(GwConverterConfig::WindMapping::TWA_TWS);
                if (mapping.valid){
                    SetN2kWindSpeed(n2kMsg, 1, WindSpeed, twa, mapping.n2kType);
                    send(n2kMsg,msg.sourceId,String(n2kMsg.PGN)+String((int)mapping.n2kType));
                }
                const GwConverterConfig::WindMapping mapping2=config.findWindMapping(GwConverterConfig::WindMapping::TWD_TWS);
                if (mapping2.valid){
                    SetN2kWindSpeed(n2kMsg, 1, WindSpeed, WindDirection, mapping2.n2kType);
                    send(n2kMsg,msg.sourceId,String(n2kMsg.PGN)+String((int)mapping2.n2kType));
                }
            }
        }
    }

    void convertHDM(const SNMEA0183Msg &msg){
        double HDM=NMEA0183DoubleNA;
        if (!NMEA0183ParseHDM_nc(msg, HDM))
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse HDM %s", msg.line);
            return;
        }
        if (! UD(HDM)) return;
        tN2kMsg n2kMsg;
        SetN2kMagneticHeading(n2kMsg,1,HDM,
            boatData->VAR->getDataWithDefault(N2kDoubleNA),
            boatData->DEV->getDataWithDefault(N2kDoubleNA)
        );
        send(n2kMsg,msg.sourceId,"127250M");    
    }
    
    void convertHDT(const SNMEA0183Msg &msg){
        double HDT=NMEA0183DoubleNA;
        if (!NMEA0183ParseHDT_nc(msg, HDT))
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse HDT %s", msg.line);
            return;
        }
        if (! UD(HDT)) return;
        tN2kMsg n2kMsg;
        SetN2kTrueHeading(n2kMsg,1,HDT);
        send(n2kMsg,msg.sourceId);    
    }
    
    void convertHDG(const SNMEA0183Msg &msg){
        double HDM=NMEA0183DoubleNA;
        double DEV=NMEA0183DoubleNA;
        double VAR=NMEA0183DoubleNA;
        if (msg.FieldCount() < 5)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse HDG %s", msg.line);
            return;
        }
        if (msg.FieldLen(0)>0){
            HDM=formatDegToRad(atof(msg.Field(0)));
        }
        else{
            return;
        }
        if (msg.FieldLen(1)>0){
            DEV=formatDegToRad(atof(msg.Field(1)));
            if (msg.Field(2)[0] == 'W') DEV=-DEV;
        }
        if (msg.FieldLen(3)>0){
            VAR=formatDegToRad(atof(msg.Field(3)));
            if (msg.Field(4)[0] == 'W') VAR=-VAR;
        }

        if (! UD(HDM)) return;
        UD(VAR);
        UD(DEV);
        tN2kMsg n2kMsg;
        SetN2kMagneticHeading(n2kMsg,1,HDM,DEV,VAR);
        send(n2kMsg,msg.sourceId,"127250M");    
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
            if (! boatData->DBS->update(DepthBelowTransducer+Offset,msg.sourceId)) return;
        }
        if (Offset == NMEA0183DoubleNA) Offset=N2kDoubleNA;
        if (! boatData->DBT->update(DepthBelowTransducer,msg.sourceId)) return;
        tN2kMsg n2kMsg;
        SetN2kWaterDepth(n2kMsg,1,DepthBelowTransducer,Offset);
        send(n2kMsg,msg.sourceId,String(n2kMsg.PGN)+String((Offset != N2kDoubleNA)?1:0));
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
                char du=msg.Field(i+1)[0];
                switch(du){
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
                    if (! boatData->DBT->update(Depth,msg.sourceId)) return;
                    tN2kMsg n2kMsg;
                    SetN2kWaterDepth(n2kMsg,1,Depth,N2kDoubleNA); 
                    send(n2kMsg,msg.sourceId,String(n2kMsg.PGN)+String(0));
                    return;   
                }
                //we can only send if we have a valid depth beloww tranducer
                //to compute the offset
                if (! boatData->DBT->isValid()) return;
                double dbs=boatData->DBT->getData();
                double offset=Depth-dbs;
                if (offset >= 0 && dt == DBK){
                    logger->logDebug(GwLog::DEBUG, "strange DBK - more depth then transducer %s", msg.line);    
                    return;
                }
                if (offset < 0 && dt == DBS){
                    logger->logDebug(GwLog::DEBUG, "strange DBS - less depth then transducer %s", msg.line);    
                    return;
                }
                if (dt == DBS){
                    if (! boatData->DBS->update(Depth,msg.sourceId)) return;
                }
                tN2kMsg n2kMsg;
                SetN2kWaterDepth(n2kMsg,1,dbs,offset); //on the N2K side we always have depth below transducer
                send(n2kMsg,msg.sourceId,(n2kMsg.PGN)+String((offset >=0)?1:0));
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
    #define validInstance(name) (name >= 0 && name <= 253)
    void convertRSA(const SNMEA0183Msg &msg){
        double RPOS=NMEA0183DoubleNA;
        double PRPOS=NMEA0183DoubleNA;
        if (msg.FieldCount() < 4)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse RSA %s", msg.line);
            return;
        }
        tN2kMsg n2kMsg;
        if (msg.FieldLen(0)>0){
            if (msg.Field(1)[0] == 'A'){
                RPOS=degToRad*atof(msg.Field(0));
                if (UD(RPOS) && validInstance(config.starboardRudderInstance)) {
                    SetN2kRudder(n2kMsg,RPOS,config.starboardRudderInstance);
                    send(n2kMsg,msg.sourceId,"127245S");
                }
            }
        }
        if (msg.FieldLen(2)>0){
            if (msg.Field(3)[0] == 'A'){
                PRPOS=degToRad*atof(msg.Field(2));
                if (UD(PRPOS) && validInstance(config.portRudderInstance)){
                    SetN2kRudder(n2kMsg,PRPOS,config.portRudderInstance);
                    send(n2kMsg,msg.sourceId,"127245P");
                }
            }
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
        tN2kMsg n2kMsg;
        if (updateDouble(boatData->HDT,TrueHeading,msg.sourceId)){
            SetN2kTrueHeading(n2kMsg,1,TrueHeading);
            send(n2kMsg,msg.sourceId); 
        }
        if(updateDouble(boatData->HDM,MagneticHeading,msg.sourceId)){
            SetN2kMagneticHeading(n2kMsg,1,MagneticHeading,
                boatData->DEV->getDataWithDefault(N2kDoubleNA),
                boatData->VAR->getDataWithDefault(N2kDoubleNA)
                );
            send(n2kMsg,msg.sourceId,"127250M"); //ensure both mag and true are sent
        }
        if (! updateDouble(boatData->STW,STW,msg.sourceId)) return;
        SetN2kBoatSpeed(n2kMsg,1,STW);
        send(n2kMsg,msg.sourceId);

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
        send(n2kMsg,msg.sourceId);
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
        double GpsTime=parts.tm_sec+60*parts.tm_min+3600*parts.tm_hour;
        if (! boatData->GPSD->update(DaysSince1970,msg.sourceId)) return;
        if (! boatData->GPST->update(GpsTime,msg.sourceId)) return;
        bool timezoneValid=false;
        if (msg.FieldLen(4) > 0 && msg.FieldLen(5)>0){
            Timezone=Timezone/60; //N2K has offset in minutes
            if (! boatData->TZ->update(Timezone,msg.sourceId)) return;
            timezoneValid=true;
        }
        tN2kMsg n2kMsg;
        if (timezoneValid){
            SetN2kLocalOffset(n2kMsg,DaysSince1970,GpsTime,Timezone);
            send(n2kMsg,msg.sourceId);
        }
        SetN2kSystemTime(n2kMsg,1,DaysSince1970,GpsTime);
        send(n2kMsg,msg.sourceId);
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
        if (GPSQualityIndicator == 0){
            LOG_DEBUG(GwLog::DEBUG, "quality 0 (no fix) for GGA %s", msg.line);
            return;   
        }
        if (! updateDouble(boatData->GPST,GPSTime,msg.sourceId)) return;
        if (! updateDouble(boatData->LAT,Latitude,msg.sourceId)) return;        
        if (! updateDouble(boatData->LON,Longitude,msg.sourceId)) return;    
        if (! updateDouble(boatData->ALT,Altitude,msg.sourceId)) return;
        if (! updateDouble(boatData->HDOP,HDOP,msg.sourceId)) return;    
        if (! boatData->GPSD->isValid()) return;
        tN2kMsg n2kMsg;
        tN2kGNSSmethod method=N2kGNSSm_noGNSS;
        if (GPSQualityIndicator <=5 ) method= (tN2kGNSSmethod)GPSQualityIndicator;
        SetN2kGNSS(n2kMsg,1, boatData->GPSD->getData(),
             GPSTime, Latitude, Longitude, Altitude,
                     N2kGNSSt_GPS, method,
                     SatelliteCount, HDOP, boatData->PDOP->getDataWithDefault(N2kDoubleNA), 0,
                     0, N2kGNSSt_GPS, DGPSReferenceStationID,
                     DGPSAge);
        send(n2kMsg,msg.sourceId);             
    }
    void convertGSA(const SNMEA0183Msg &msg){
        if (msg.FieldCount() < 17)
        {
            LOG_DEBUG(GwLog::DEBUG, "failed to parse GSA %s", msg.line);
            return;
        }
        int fixMode=atoi(msg.Field(1));
        if (fixMode != 2 && fixMode != 3){
            LOG_DEBUG(GwLog::DEBUG,"no fix in GSA, mode=%d",fixMode);
            return;
        }
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
        send(n2kMsg,msg.sourceId);
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
        for (int idx=3;idx < msg.FieldCount();idx+=4){
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
                send(n2kMsg,msg.sourceId);
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
        if (! updateDouble(boatData->LAT,GLL.latitude,msg.sourceId)) return;
        if (! updateDouble(boatData->LON,GLL.longitude,msg.sourceId)) return;
        if (! updateDouble(boatData->GPST,GLL.GPSTime,msg.sourceId)) return;
        tN2kMsg n2kMsg;
        SetN2kLatLonRapid(n2kMsg,GLL.latitude,GLL.longitude);
        send(n2kMsg,msg.sourceId);
    }

    void convertROT(const SNMEA0183Msg &msg){
        double ROT=NMEA0183DoubleNA;
        if (! NMEA0183ParseROT_nc(msg,ROT)){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse ROT %s",msg.line);
            return;
        }
        ROT=ROT / ROT_WA_FACTOR; 
        if (! updateDouble(boatData->ROT,ROT,msg.sourceId)) return;
        tN2kMsg n2kMsg;
        SetN2kRateOfTurn(n2kMsg,1,ROT);
        send(n2kMsg,msg.sourceId);
    }
    void convertXTE(const SNMEA0183Msg &msg){
        if (msg.FieldCount() < 6){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse XTE %s",msg.line);
            return; 
        }
        if (msg.Field(0)[0] != 'A') return;
        if (msg.Field(1)[0] != 'A') return;
        if (msg.Field(4)[0] != 'N') return; //nm only
        if (msg.FieldLen(2) < 1) return;
        const char dir=msg.Field(3)[0];
        if (dir != 'L' && dir != 'R') return;
        double xte=atof(msg.Field(2)) * nmTom;
        if (dir == 'R') xte=-xte;
        if (! updateDouble(boatData->XTE,xte,msg.sourceId)) return;
        tN2kMsg n2kMsg;
        tN2kXTEMode mode=xteMode(msg.Field(5)[0]);
        SetN2kXTE(n2kMsg,1,mode,false,xte);
        send(n2kMsg,msg.sourceId);
    }

    void convertMTW(const SNMEA0183Msg &msg){
        if (msg.FieldCount() < 2){
            LOG_DEBUG(GwLog::DEBUG,"unable to parse MTW %s",msg.line);
            return;   
        }
        if (msg.Field(1)[0] != 'C'){
            LOG_DEBUG(GwLog::DEBUG,"invalid temp unit in MTW %s",msg.line);   
            return; 
        }
        if (msg.FieldLen(0) < 1) return;
        double WTemp=CToKelvin(atof(msg.Field(0)));
        UD(WTemp);
        tN2kMsg n2kMsg;
        SetN2kPGN130310(n2kMsg,1,WTemp);
        send(n2kMsg,msg.sourceId);
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
        converters.registerConverter(
            129283UL,
            String(F("XTE")), &NMEA0183DataToN2KFunctions::convertXTE); 
        converters.registerConverter(
            130310UL,
            String(F("MTW")), &NMEA0183DataToN2KFunctions::convertMTW);             
        unsigned long *xdrpgns=new unsigned long[8]{127505UL,127508UL,130312UL,130313UL,130314UL,127489UL,127488UL,127257UL};    
        converters.registerConverter(
            8,
            xdrpgns,
            F("XDR"), &NMEA0183DataToN2KFunctions::convertXDR);
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
            if (!msg.SetMessageCor(buffer))
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

    NMEA0183DataToN2KFunctions(GwLog *logger, GwBoatData *boatData, N2kSender callback, 
        GwXDRMappings *xdrMappings,
        const GwConverterConfig &cfg)
        : NMEA0183DataToN2K(logger, boatData, callback)
    {
        this->config=cfg;
        this->xdrMappings=xdrMappings;
        aisDecoder= new MyAisDecoder(logger,this->sender);
        registerConverters();
        LOG_DEBUG(GwLog::LOG, "NMEA0183DataToN2KFunctions: registered %d converters", converters.numConverters());
    }
    };

NMEA0183DataToN2K* NMEA0183DataToN2K::create(GwLog *logger,GwBoatData *boatData,N2kSender callback,
    GwXDRMappings *xdrMappings,
    const GwConverterConfig &config){
    return new NMEA0183DataToN2KFunctions(logger, boatData,callback,xdrMappings,config);

}
