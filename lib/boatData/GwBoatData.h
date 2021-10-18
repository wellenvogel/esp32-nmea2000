#ifndef _GWBOATDATA_H
#define _GWBOATDATA_H

#include "GwLog.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <map>
#define GW_BOAT_VALUE_LEN 32
class GwBoatItem{
    private:
        char value[GW_BOAT_VALUE_LEN];
        long lastSet;
        void uls(){
            lastSet=millis();
        }
    public:
        const char * getValue() const{return value;}
        long getLastSet() const {return lastSet;}
        bool isValid(long minTime) const {return lastSet > minTime;}
        GwBoatItem(){
            value[0]=0;
            lastSet=-1;
        }
        void update(String nv){
            strncpy(value,nv.c_str(),GW_BOAT_VALUE_LEN-1);
            uls();
        }
        void update(const char * nv){
            strncpy(value,nv,GW_BOAT_VALUE_LEN-1);
        }
        void update(long nv){
            ltoa(nv,value,10);
            uls();
        }
        void update(bool nv){
            if (nv) strcpy_P(value,PSTR("true"));
            else strcpy_P(value,PSTR("false"));
            uls();
        }
        void update(double nv){
            dtostrf(nv,3,7,value);
            uls();
        }
        void invalidate(){
            lastSet=0;
        }

};
class GwBoatData{
    typedef std::map<String,GwBoatItem*> GwBoatItemMap;
    private:
        const long maxAge=60000; //max age for valid data in ms
        GwLog *logger;
        GwBoatItemMap values;
        GwBoatItem *find(String name, bool doCreate=true);
    public:
        GwBoatData(GwLog *logger);
        ~GwBoatData();    
        void update(String name,const char *value);
        void update(String name, String value);
        void update(String name, long value);
        void update(String name, bool value);
        void update(String name, double value);
        String toJson() const;

};
#endif