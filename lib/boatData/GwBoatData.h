#ifndef _GWBOATDATA_H
#define _GWBOATDATA_H

#include "GwLog.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <map>
#define GW_BOAT_VALUE_LEN 32
class GwBoatItemBase{
    public:
        static const long INVALID_TIME=60000;
        typedef std::map<String,GwBoatItemBase*> GwBoatItemMap;
    protected:
        long lastSet;
        long invalidTime=INVALID_TIME;
        void uls(){
            lastSet=millis();
        }
    public:
        long getLastSet() const {return lastSet;}
        bool isValid(long now) const {
            return (lastSet + invalidTime) >= now;
        }
        GwBoatItemBase(long invalidTime=INVALID_TIME){
            lastSet=-1;
            this->invalidTime=invalidTime;
        }
        virtual ~GwBoatItemBase(){}
        void invalidate(){
            lastSet=0;
        }
        virtual void toJsonDoc(DynamicJsonDocument *doc,String name)=0;
};
class GwBoatData{
    private:
        GwLog *logger;
        GwBoatItemBase::GwBoatItemMap values;
    public:
        GwBoatData(GwLog *logger);
        ~GwBoatData();
        GwBoatItemBase::GwBoatItemMap * getValues();    
        String toJson() const;
    friend class GwBoatItemBase;    
};
template<class T> class GwBoatItem : public GwBoatItemBase{
    private:
        T data;
    public:
        GwBoatItem(long invalidTime=INVALID_TIME): GwBoatItemBase(invalidTime){}
        virtual ~GwBoatItem(){}
        void update(T nv){
            data=nv;
            uls();
        }
        T getData(){
            return data;
        }
        virtual void toJsonDoc(DynamicJsonDocument *doc,String name){
            (*doc)[name]=data;
        }
        static GwBoatItem<T> *findOrCreate(GwBoatData *handler, String name,bool doCreate=true, 
            long invalidTime=GwBoatItemBase::INVALID_TIME){
            GwBoatItemMap *values=handler->getValues();    
            GwBoatItemMap::iterator it;
            if ((it=values->find(name)) != values->end()){
                return (GwBoatItem<T> *)it->second;
            }
            if (! doCreate) return NULL;
            GwBoatItem<T> *ni=new GwBoatItem<T>(invalidTime);
            (*values)[name]=ni;
            return ni; 
        }
};

#endif