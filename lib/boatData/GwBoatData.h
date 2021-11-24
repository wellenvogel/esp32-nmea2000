#ifndef _GWBOATDATA_H
#define _GWBOATDATA_H

#include "GwLog.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <map>
#define GW_BOAT_VALUE_LEN 32
#define GWSC(name) static constexpr const __FlashStringHelper* name=F(#name)
#define GWTYPE_DOUBLE 1
#define GWTYPE_UINT32 2
#define GWTYPE_UINT16 3
#define GWTYPE_INT16 4
#define GWTYPE_USER 100
class GwBoatItemTypes{
    public:
        static int getType(const uint32_t &x){return GWTYPE_UINT32;}
        static int getType(const uint16_t &x){return GWTYPE_UINT16;}
        static int getType(const int16_t &x){return GWTYPE_INT16;}
        static int getType(const double &x){return GWTYPE_DOUBLE;}
};
class GwBoatItemBase{
    public:
        static const unsigned long INVALID_TIME=60000;
        //the formatter names that must be known in js
        GWSC(formatCourse);
        GWSC(formatKnots);
        GWSC(formatWind);
        GWSC(formatLatitude);
        GWSC(formatLongitude);
        GWSC(formatXte);
        GWSC(formatFixed0);
        GWSC(formatDepth);
        GWSC(kelvinToC);
        GWSC(mtr2nm);
        GWSC(formatDop);
        GWSC(formatRot);
        typedef std::map<String,GwBoatItemBase*> GwBoatItemMap;
    protected:
        int type;
        unsigned long lastSet=0;
        unsigned long invalidTime=INVALID_TIME;
        String name;
        String format;
        void uls(unsigned long ts=0){
            if (ts) lastSet=ts;
            else lastSet=millis();
        }
    public:
        int getCurrentType(){return type;}
        unsigned long getLastSet() const {return lastSet;}
        bool isValid(unsigned long now=0) const {
            if (lastSet == 0) return false;
            if (invalidTime == 0) return true;
            if (now == 0) now=millis();
            return (lastSet + invalidTime) >= now;
        }
        GwBoatItemBase(String name,String format,unsigned long invalidTime=INVALID_TIME){
            lastSet=0;
            this->invalidTime=invalidTime;
            this->name=name;
            this->format=format;
            this->type=0;
        }
        virtual ~GwBoatItemBase(){}
        void invalidate(){
            lastSet=0;
        }
        virtual void toJsonDoc(JsonDocument *doc, unsigned long minTime)=0;
        virtual size_t getJsonSize(){return JSON_OBJECT_SIZE(15);}
        virtual int getLastSource()=0;
        virtual void refresh(unsigned long ts=0){uls(ts);}
        String getName(){return name;}
};
class GwBoatData;
template<class T> class GwBoatItem : public GwBoatItemBase{
    protected:
        T data;
        int lastUpdateSource;
    public:
        GwBoatItem(int type,String name,String formatInfo,unsigned long invalidTime=INVALID_TIME,GwBoatItemMap *map=NULL):
            GwBoatItemBase(name,formatInfo,invalidTime){
            this->type=type;
            if (map){
                (*map)[name]=this;
            }
            lastUpdateSource=-1;
        }
        virtual ~GwBoatItem(){}
        bool update(T nv, int source=-1){
            unsigned long now=millis();
            if (isValid(now)){
                //priority handling
                //sources with lower ids will win
                //and we will not overwrite their value
                if (lastUpdateSource < source && lastUpdateSource >= 0){
                    return false;
                }
            }
            data=nv;
            lastUpdateSource=source;
            uls(now);
            return true;
        }
        bool updateMax(T nv,int sourceId=-1){
            unsigned long now=millis();
            if (! isValid(now)){
                return update(nv,sourceId);
            }
            if (getData() < nv){
                data=nv;
                lastUpdateSource=sourceId;
                uls(now);
                return true;
            }
            return false;
        }
        T getData(){
            return data;
        }
        T getDataWithDefault(T defaultv){
            if (! isValid(millis())) return defaultv;
            return data;
        }
        virtual void toJsonDoc(JsonDocument *doc, unsigned long minTime){
            JsonObject o=doc->createNestedObject(name);
            o[F("value")]=getData();
            o[F("update")]=minTime-lastSet;
            o[F("source")]=lastUpdateSource;
            o[F("valid")]=isValid(minTime);
            o[F("format")]=format;
        }
        virtual int getLastSource(){return lastUpdateSource;}
};
double formatCourse(double cv);
double formatDegToRad(double deg);
double formatWind(double cv);
double formatKnots(double cv);
uint32_t mtr2nm(uint32_t m);
double mtr2nm(double m);

class GwSatInfo{
    public:
        unsigned char PRN;
        uint32_t Elevation;
        uint32_t Azimut;
        uint32_t SNR;
        unsigned long timestamp;
};
class GwSatInfoList{
    public:
        static const unsigned long lifeTime=32000;
        std::vector<GwSatInfo> sats;
        void houseKeeping(unsigned long ts=0){
            if (ts == 0) ts=millis();
            sats.erase(std::remove_if(
                sats.begin(),
                sats.end(),
                [ts,this](const GwSatInfo &info){
                    return (info.timestamp + lifeTime) < ts;
                    }
            ),sats.end());   
        }
        void update(GwSatInfo entry){
            unsigned long now=millis();
            entry.timestamp=now;
            for (auto it=sats.begin();it!=sats.end();it++){
                if (it->PRN == entry.PRN){
                    *it=entry;
                    houseKeeping();
                    return;
                }
            }
            houseKeeping();
            sats.push_back(entry);
        }
        int getNumSats() const{
            return sats.size();
        }
        GwSatInfo *getAt(int idx){
            if (idx >= 0 && idx < sats.size()) return &sats.at(idx);
            return NULL;
        }
};

bool convertToJson(const GwSatInfoList &si,JsonVariant &variant);
class GwBoatDataSatList : public GwBoatItem<GwSatInfoList>
{
public:
    GwBoatDataSatList(String name, String formatInfo, unsigned long invalidTime = INVALID_TIME, GwBoatItemMap *map = NULL) : 
        GwBoatItem<GwSatInfoList>(GWTYPE_USER+1, name, formatInfo, invalidTime, map) {}
    bool update(GwSatInfo info, int source)
    {
        unsigned long now = millis();
        if (isValid(now))
        {
            //priority handling
            //sources with lower ids will win
            //and we will not overwrite their value
            if (lastUpdateSource < source)
            {
                return false;
            }
        }
        lastUpdateSource = source;
        uls(now);
        data.update(info);
        return true;
    }
    virtual void toJsonDoc(JsonDocument *doc, unsigned long minTime){
            data.houseKeeping();
            GwBoatItem<GwSatInfoList>::toJsonDoc(doc,minTime);
    }
    GwSatInfo *getAt(int idx){
        if (! isValid()) return NULL;
        return data.getAt(idx);
    }
    int getNumSats(){
        if (! isValid()) return 0;
        return data.getNumSats();
    }

};

class GwBoatItemNameProvider
{
public:
    virtual String getBoatItemName() = 0;
    virtual String getBoatItemFormat() = 0;
    virtual unsigned long getInvalidTime(){ return GwBoatItemBase::INVALID_TIME;}
    virtual ~GwBoatItemNameProvider() {}
};
#define GWBOATDATA(type,name,time,fmt)  \
    GwBoatItem<type> *name=new GwBoatItem<type>(GwBoatItemTypes::getType((type)0),F(#name),GwBoatItemBase::fmt,time,&values) ;
#define GWSPECBOATDATA(clazz,name,time,fmt)  \
    clazz *name=new clazz(F(#name),GwBoatItemBase::fmt,time,&values) ;
class GwBoatData{
    private:
        GwLog *logger;
        GwBoatItemBase::GwBoatItemMap values;
    public:

    GWBOATDATA(double,COG,4000,formatCourse)
    GWBOATDATA(double,TWD,4000,formatCourse)
    GWBOATDATA(double,AWD,4000,formatCourse)
    GWBOATDATA(double,SOG,4000,formatKnots)
    GWBOATDATA(double,STW,4000,formatKnots)
    GWBOATDATA(double,TWS,4000,formatKnots)
    GWBOATDATA(double,AWS,4000,formatKnots)
    GWBOATDATA(double,MaxTws,0,formatKnots)
    GWBOATDATA(double,MaxAws,0,formatKnots)
    GWBOATDATA(double,AWA,4000,formatWind)
    GWBOATDATA(double,Heading,4000,formatCourse) //true
    GWBOATDATA(double,MagneticHeading,4000,formatCourse)
    GWBOATDATA(double,ROT,4000,formatRot)
    GWBOATDATA(double,Variation,4000,formatCourse)
    GWBOATDATA(double,Deviation,4000,formatCourse)
    GWBOATDATA(double,HDOP,4000,formatDop)
    GWBOATDATA(double,PDOP,4000,formatDop)
    GWBOATDATA(double,VDOP,4000,formatDop)
    GWBOATDATA(double,RudderPosition,4000,formatCourse)
    GWBOATDATA(double,Latitude,4000,formatLatitude)
    GWBOATDATA(double,Longitude,4000,formatLongitude)
    GWBOATDATA(double,Altitude,4000,formatFixed0)
    GWBOATDATA(double,WaterDepth,4000,formatDepth)
    GWBOATDATA(double,DepthTransducer,4000,formatDepth)
    GWBOATDATA(double,SecondsSinceMidnight,4000,formatFixed0)
    GWBOATDATA(double,WaterTemperature,4000,kelvinToC)
    GWBOATDATA(double,XTE,4000,formatXte)
    GWBOATDATA(double,DTW,4000,mtr2nm)
    GWBOATDATA(double,BTW,4000,formatCourse)
    GWBOATDATA(double,WPLatitude,4000,formatLatitude)
    GWBOATDATA(double,WPLongitude,4000,formatLongitude)
    GWBOATDATA(uint32_t,Log,16000,mtr2nm)
    GWBOATDATA(uint32_t,TripLog,16000,mtr2nm)
    GWBOATDATA(uint32_t,DaysSince1970,4000,formatFixed0)
    GWBOATDATA(int16_t,Timezone,8000,formatFixed0)
    GWSPECBOATDATA(GwBoatDataSatList,SatInfo,GwSatInfoList::lifeTime,formatFixed0);
    public:
        GwBoatData(GwLog *logger);
        ~GwBoatData();
        template<class T> GwBoatItem<T> *getOrCreate(T initial,GwBoatItemNameProvider *provider);
        template<class T> bool update(T value,int source,GwBoatItemNameProvider *provider);
        template<class T> T getDataWithDefault(T defaultv, GwBoatItemNameProvider *provider);
        String toJson() const;
};


#endif