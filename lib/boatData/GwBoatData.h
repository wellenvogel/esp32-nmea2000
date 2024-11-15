#ifndef _GWBOATDATA_H
#define _GWBOATDATA_H

#include "GwLog.h"
#include "GWConfig.h"
#include <Arduino.h>
#include <map>
#include <vector>
#define GW_BOAT_VALUE_LEN 32
#define GWSC(name) static constexpr const char* name=#name

//see https://github.com/wellenvogel/esp32-nmea2000/issues/44
//factor to convert from N2k/SI rad/s to current NMEA rad/min
#define ROT_WA_FACTOR 60

class GwJsonDocument;
class GwBoatData;

class GwBoatItemBase{
    public:
        using TOType=enum{
            def=1,
            ais=2,
            sensor=3,
            lng=4,
            user=5,
            keep=6
        };
        class StringWriter{
            uint8_t *buffer =NULL;
            uint8_t *wp=NULL;
            size_t bufferSize=0;
            size_t baseOffset=0;
            void ensure(size_t size);
            public:
                StringWriter();
                size_t write(uint8_t c);
                size_t write(const uint8_t* s, size_t n);
                const char * c_str() const;
                int getSize() const;
                void setBase();
                bool baseFilled();
                void reset();
        };
        static const long INVALID_TIME=60000;
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
        GWSC(formatDate);
        GWSC(formatTime);
    protected:
        int type;
        unsigned long lastSet=0;
        long invalidTime=INVALID_TIME;
        String name;
        String format;
        StringWriter writer;
        TOType toType=TOType::def;
        void uls(unsigned long ts=0){
            if (ts) lastSet=ts;
            else lastSet=millis();
            writer.reset(); //value has changed
        }
        int lastUpdateSource;
    public:
        int getCurrentType(){return type;}
        unsigned long getLastSet() const {return lastSet;}
        bool isValid(unsigned long now=0) const ;
        GwBoatItemBase(String name,String format,TOType toType);
        GwBoatItemBase(String name,String format,unsigned long invalidTime);
        virtual ~GwBoatItemBase(){}
        void invalidate(){
            lastSet=0;
        }
        const char *getDataString(){
            fillString();
            return writer.c_str();
            }
        virtual void fillString()=0;
        virtual void toJsonDoc(GwJsonDocument *doc, unsigned long minTime)=0;
        virtual size_t getJsonSize();
        virtual int getLastSource(){return lastUpdateSource;}
        virtual void refresh(unsigned long ts=0){uls(ts);}
        virtual double getDoubleValue()=0;
        String getName(){return name;}
        const String & getFormat() const{return format;}
        virtual void setInvalidTime(GwConfigHandler *cfg);
        TOType getToType(){return toType;}
        class GwBoatItemMap : public std::map<String,GwBoatItemBase*>{
            GwBoatData *boatData;
            public:
            GwBoatItemMap(GwBoatData *bd):boatData(bd){}
            void add(const String &name,GwBoatItemBase *item);
        };
};
template<class T> class GwBoatItem : public GwBoatItemBase{
    protected:
        T data;
        bool lastStringValid=false;
    public:
        GwBoatItem(String name,String formatInfo,unsigned long invalidTime=INVALID_TIME,GwBoatItemMap *map=NULL);
        GwBoatItem(String name,String formatInfo,TOType toType,GwBoatItemMap *map=NULL);
        virtual ~GwBoatItem(){}
        bool update(T nv, int source);
        bool updateMax(T nv,int sourceId);
        T getData(){
            return data;
        }
        T getDataWithDefault(T defaultv){
            if (! isValid(millis())) return defaultv;
            return data;
        }
        virtual double getDoubleValue(){return (double)data;}
        virtual void fillString();
        virtual void toJsonDoc(GwJsonDocument *doc, unsigned long minTime);
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
        unsigned long validTill;
};
class GwSatInfoList{
    public:
        static const GwBoatItemBase::TOType toType=GwBoatItemBase::TOType::lng;
        std::vector<GwSatInfo> sats;
        void houseKeeping(unsigned long ts=0);
        void update(GwSatInfo entry, unsigned long validTill);
        int getNumSats() const{
            return sats.size();
        }
        GwSatInfo *getAt(int idx){
            if (idx >= 0 && idx < sats.size()) return &sats.at(idx);
            return NULL;
        }
        operator double(){ return getNumSats();}
};

class GwBoatDataSatList : public GwBoatItem<GwSatInfoList>
{
public:
    GwBoatDataSatList(String name, String formatInfo, GwBoatItemBase::TOType toType, GwBoatItemMap *map = NULL);
    bool update(GwSatInfo info, int source);
    virtual void toJsonDoc(GwJsonDocument *doc, unsigned long minTime);
    GwSatInfo *getAt(int idx){
        if (! isValid()) return NULL;
        return data.getAt(idx);
    }
    int getNumSats(){
        if (! isValid()) return 0;
        return data.getNumSats();
    }
    virtual double getDoubleValue(){
        return (double)(data.getNumSats());
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
#define GWBOATDATAT(type,name,toType,fmt)  \
    static constexpr const char* _##name=#name; \
    GwBoatItem<type> *name=new GwBoatItem<type>(#name,GwBoatItemBase::fmt,toType,&values) ;
#define GWBOATDATA(type,name,fmt) GWBOATDATAT(type,name,GwBoatItemBase::TOType::def,fmt) 
#define GWSPECBOATDATA(clazz,name,toType,fmt)  \
    clazz *name=new clazz(#name,GwBoatItemBase::fmt,toType,&values) ;
class GwBoatData{
    private:
        GwLog *logger=nullptr;
        GwConfigHandler *config=nullptr;
        GwBoatItemBase::GwBoatItemMap values{this};
    public:

    GWBOATDATA(double,COG,formatCourse) // course over ground
    GWBOATDATA(double,SOG,formatKnots) // speed over ground
    GWBOATDATA(double,HDT,formatCourse) // true heading
    GWBOATDATA(double,HDM,formatCourse) // magnetic heading
    GWBOATDATA(double,STW,formatKnots) // water speed
    GWBOATDATA(double,VAR,formatWind) // variation
    GWBOATDATA(double,DEV,formatWind) // deviation
    GWBOATDATA(double,AWA,formatWind) // apparent wind ANGLE
    GWBOATDATA(double,AWS,formatKnots) // apparent wind speed
    GWBOATDATAT(double,MaxAws,GwBoatItemBase::TOType::keep,formatKnots)
    GWBOATDATA(double,TWD,formatCourse) // true wind DIRECTION
    GWBOATDATA(double,TWA,formatWind) // true wind ANGLE
    GWBOATDATA(double,TWS,formatKnots) // true wind speed

    GWBOATDATAT(double,MaxTws,GwBoatItemBase::TOType::keep,formatKnots)
    GWBOATDATA(double,ROT,formatRot) // rate of turn
    GWBOATDATA(double,RPOS,formatWind) // rudder position
    GWBOATDATA(double,PRPOS,formatWind) // secondary rudder position
    GWBOATDATA(double,LAT,formatLatitude) 
    GWBOATDATA(double,LON,formatLongitude)
    GWBOATDATA(double,ALT,formatFixed0) //altitude
    GWBOATDATA(double,HDOP,formatDop)
    GWBOATDATA(double,PDOP,formatDop)
    GWBOATDATA(double,VDOP,formatDop)
    GWBOATDATA(double,DBS,formatDepth) //waterDepth (below surface)
    GWBOATDATA(double,DBT,formatDepth) //DepthTransducer
    GWBOATDATA(double,GPST,formatTime) // GPS time (seconds of day)
    GWBOATDATA(uint32_t,GPSD,formatDate) // GPS date (days since 1979-01-01)
    GWBOATDATAT(int16_t,TZ,GwBoatItemBase::TOType::lng,formatFixed0)
    GWBOATDATA(double,WTemp,kelvinToC)
    GWBOATDATAT(uint32_t,Log,GwBoatItemBase::TOType::lng,mtr2nm)
    GWBOATDATAT(uint32_t,TripLog,GwBoatItemBase::TOType::lng,mtr2nm)
    GWBOATDATA(double,DTW,mtr2nm) // distance to waypoint
    GWBOATDATA(double,BTW,formatCourse) // bearing to waypoint
    GWBOATDATA(double,XTE,formatXte) // cross track error
    GWBOATDATA(double,WPLat,formatLatitude) // waypoint latitude
    GWBOATDATA(double,WPLon,formatLongitude) // waypoint longitude
    GWSPECBOATDATA(GwBoatDataSatList,SatInfo,GwSatInfoList::toType,formatFixed0);
    public:
        GwBoatData(GwLog *logger, GwConfigHandler *cfg);
        ~GwBoatData();
        void begin();
        template<class T> GwBoatItem<T> *getOrCreate(T initial,GwBoatItemNameProvider *provider);
        template<class T> bool update(T value,int source,GwBoatItemNameProvider *provider);
        template<class T> T getDataWithDefault(T defaultv, GwBoatItemNameProvider *provider);
        void setInvalidTime(GwBoatItemBase *item);
        bool isValid(String name);
        double getDoubleValue(String name,double defaultv);
        GwBoatItemBase *getBase(String name);
        String toJson() const;
        String toString();
};


#endif