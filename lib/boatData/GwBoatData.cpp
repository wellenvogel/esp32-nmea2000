#include "GwBoatData.h"

GwBoatData::GwBoatData(GwLog *logger){
    
}
GwBoatData::~GwBoatData(){
    GwBoatItemBase::GwBoatItemMap::iterator it;
    for (it=values.begin() ; it != values.end();it++){
        delete it->second;
    }
}

String GwBoatData::toJson() const {
    long minTime=millis();
    DynamicJsonDocument json(800);
    GwBoatItemBase::GwBoatItemMap::const_iterator it;
    for (it=values.begin() ; it != values.end();it++){
        if (it->second->isValid(minTime)){
            it->second->toJsonDoc(&json,it->first);
        }
    }
    String buf;
    serializeJson(json,buf);
    return buf;
}

GwBoatItemBase::GwBoatItemMap * GwBoatData::getValues(){
    return &values;
}