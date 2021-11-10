#ifndef _GWBOATDATA_H
#define _GWBOATDATA_H

#include "GwLog.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <vector>
#define GW_BOAT_VALUE_LEN 32
#define GWSC(name) static constexpr const __FlashStringHelper* name=F(#name)
class GwBoatItemBase{
    public:
        static const unsigned long INVALID_TIME=60000;
        //the formatter names that must be known in js
        GWSC(formatCourse);
        GWSC(formatKnots);
        GWSC(formatWind);
        GWSC(formatLatitude);
        GWSC(formatLongitude);
        GWSC(formatFixed0);
        GWSC(formatDepth);
        GWSC(kelvinToC);
        GWSC(mtr2nm);
        typedef std::vector<GwBoatItemBase*> GwBoatItemMap;
    protected:
        unsigned long lastSet=0;
        unsigned long invalidTime=INVALID_TIME;
        String name;
        String format;
        void uls(unsigned long ts=0){
            if (ts) lastSet=ts;
            else lastSet=millis();
        }
    public:
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
        }
        virtual ~GwBoatItemBase(){}
        void invalidate(){
            lastSet=0;
        }
        virtual void toJsonDoc(JsonDocument *doc, unsigned long minTime)=0;
        virtual size_t getJsonSize(){return JSON_OBJECT_SIZE(15);}
        virtual int getLastSource()=0;
};
class GwBoatData;
template<class T> class GwBoatItem : public GwBoatItemBase{
    private:
        T data;
        int lastUpdateSource;
    public:
        GwBoatItem(String name,String formatInfo,unsigned long invalidTime=INVALID_TIME,GwBoatItemMap *map=NULL):
            GwBoatItemBase(name,formatInfo,invalidTime){
            if (map){
                map->push_back(this);
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
                if (lastUpdateSource < source){
                    return false;
                }
            }
            data=nv;
            lastUpdateSource=source;
            uls(now);
            return true;
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


#define GWBOATDATA(type,name,time,fmt)  \
    GwBoatItem<type> *name=new GwBoatItem<type>(F(#name),GwBoatItemBase::fmt,time,&values) ;
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
    GWBOATDATA(double,Variation,4000,formatCourse)
    GWBOATDATA(double,Deviation,4000,formatCourse)
    GWBOATDATA(double,RudderPosition,4000,formatCourse)
    GWBOATDATA(double,Latitude,4000,formatLatitude)
    GWBOATDATA(double,Longitude,4000,formatLongitude)
    GWBOATDATA(double,Altitude,4000,formatFixed0)
    GWBOATDATA(double,WaterDepth,4000,formatDepth)
    GWBOATDATA(double,DepthTransducer,4000,formatDepth)
    GWBOATDATA(double,SecondsSinceMidnight,4000,formatFixed0)
    GWBOATDATA(double,WaterTemperature,4000,kelvinToC)
    GWBOATDATA(double,XTE,4000,formatFixed0)
    GWBOATDATA(double,DTW,4000,mtr2nm)
    GWBOATDATA(double,BTW,4000,formatCourse)
    GWBOATDATA(double,WPLatitude,4000,formatLatitude)
    GWBOATDATA(double,WPLongitude,4000,formatLongitude)
    GWBOATDATA(uint32_t,Log,0,mtr2nm)
    GWBOATDATA(uint32_t,TripLog,0,mtr2nm)
    GWBOATDATA(uint32_t,DaysSince1970,4000,formatFixed0)
    public:
        GwBoatData(GwLog *logger);
        ~GwBoatData();
        String toJson() const;
};


#endif