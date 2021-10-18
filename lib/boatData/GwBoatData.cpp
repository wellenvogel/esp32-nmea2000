#include "GwBoatData.h"

GwBoatItem *GwBoatData::find(String name,bool doCreate){
    GwBoatItemMap::iterator it;
    if ((it=values.find(name)) != values.end()){
        return it->second;
    }
    if (! doCreate) return NULL;
    GwBoatItem *ni=new GwBoatItem();
    values[name]=ni;
    return ni;
}
GwBoatData::GwBoatData(GwLog *logger){
    
}
GwBoatData::~GwBoatData(){
    GwBoatItemMap::iterator it;
    for (it=values.begin() ; it != values.end();it++){
        delete it->second;
    }
}
void GwBoatData::update(String name,const char *value){
   GwBoatItem *i=find(name);
   i->update(value); 
}
void GwBoatData::update(String name, String value){
   GwBoatItem *i=find(name);
   i->update(value); 
}
void GwBoatData::update(String name, long value){
   GwBoatItem *i=find(name);
   i->update(value); 
}
void GwBoatData::update(String name, double value){
   GwBoatItem *i=find(name);
   i->update(value); 
}
void GwBoatData::update(String name, bool value){
    GwBoatItem *i=find(name);
    i->update(value); 
}
String GwBoatData::toJson() const {
    long minTime=millis() - maxAge;
    DynamicJsonDocument json(800);
    GwBoatItemMap::const_iterator it;
    for (it=values.begin() ; it != values.end();it++){
        if (it->second->isValid(minTime)){
            json[it->first]=it->second->getValue();
        }
    }
    String buf;
    serializeJson(json,buf);
    return buf;
}