#ifndef _GWBOATDATA_H
#define _GWBOATDATA_H

#include "GwLog.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <map>
#define GW_BOAT_VALUE_LEN 32
class GwBoatItemBase{
    public:
        static const unsigned long INVALID_TIME=60000;
        typedef std::map<String,GwBoatItemBase*> GwBoatItemMap;
    protected:
        unsigned long lastSet=0;
        unsigned long invalidTime=INVALID_TIME;
        void uls(){
            lastSet=millis();
        }
    public:
        unsigned long getLastSet() const {return lastSet;}
        bool isValid(unsigned long now=0) const {
            if (lastSet == 0) return false;
            if (invalidTime == 0) return true;
            if (now == 0) now=millis();
            return (lastSet + invalidTime) >= now;
        }
        GwBoatItemBase(unsigned long invalidTime=INVALID_TIME){
            lastSet=0;
            this->invalidTime=invalidTime;
        }
        virtual ~GwBoatItemBase(){}
        void invalidate(){
            lastSet=0;
        }
        virtual void toJsonDoc(DynamicJsonDocument *doc,String name)=0;
};
class GwBoatData;
template<class T> class GwBoatItem : public GwBoatItemBase{
    public:
        typedef T (*Formatter)(T);
    private:
        T data;
        Formatter fmt;
    public:
        GwBoatItem(unsigned long invalidTime=INVALID_TIME,Formatter fmt=NULL):
            GwBoatItemBase(invalidTime){
            this->fmt=fmt;
        }
        virtual ~GwBoatItem(){}
        void update(T nv){
            data=nv;
            uls();
        }
        T getData(bool useFormatter=false){
            if (! useFormatter || fmt == NULL) return data;
            return (*fmt)(data);
        }
        T getDataWithDefault(T defaultv){
            if (! isValid(millis())) return defaultv;
            return data;
        }
        virtual void toJsonDoc(DynamicJsonDocument *doc,String name){
            (*doc)[name]=getData(true);
        }
        private:
        static GwBoatItem<T> *findOrCreate(GwLog *logger,GwBoatItemMap *values, String name,bool doCreate=true, 
            long invalidTime=GwBoatItemBase::INVALID_TIME, Formatter fmt=NULL){
            GwBoatItemMap::iterator it;
            if ((it=values->find(name)) != values->end()){
                return (GwBoatItem<T> *)it->second;
            }
            if (! doCreate) return NULL;
            LOG_DEBUG(GwLog::DEBUG,"creating boat data item %s",name.c_str());
            GwBoatItem<T> *ni=new GwBoatItem<T>(invalidTime,fmt);
            (*values)[name]=ni;
            return ni; 
        }
        friend class GwBoatData;
};

/**
 * implement a get for a particular type of boatData
 * will create a method get##name##Item that returns a GwBoatItem<type>
 * */
#define GWBOATDATA_IMPL_ITEM(type,name) GwBoatItem<type> *get##name##Item(String iname,bool doCreate=true, \
                long invalidTime=GwBoatItemBase::INVALID_TIME, GwBoatItem<type>::Formatter fmt=NULL){ \
                return GwBoatItem<type>::findOrCreate(logger,&values,iname, doCreate, \
                invalidTime,fmt);\
            }

class GwBoatData{
    private:
        GwLog *logger;
        GwBoatItemBase::GwBoatItemMap values;
    public:
        GwBoatData(GwLog *logger);
        ~GwBoatData();
        String toJson() const;
        GWBOATDATA_IMPL_ITEM(double,Double)
        GWBOATDATA_IMPL_ITEM(long,Long)
        GWBOATDATA_IMPL_ITEM(uint32_t,Uint32)     
};


#endif