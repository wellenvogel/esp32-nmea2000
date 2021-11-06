#include "GwBoatData.h"

GwBoatData::GwBoatData(GwLog *logger){
    this->logger=logger;
}
GwBoatData::~GwBoatData(){
    GwBoatItemBase::GwBoatItemMap::iterator it;
    for (it=values.begin() ; it != values.end();it++){
        delete *it;
    }
}

String GwBoatData::toJson() const {
    unsigned long minTime=millis();
    GwBoatItemBase::GwBoatItemMap::const_iterator it;
    size_t count=0;
    size_t elementSizes=0;
    for (it=values.begin() ; it != values.end();it++){
        count++;
        elementSizes+=(*it)->getJsonSize();
    }
    DynamicJsonDocument json(JSON_OBJECT_SIZE(count)+elementSizes+10);
    for (it=values.begin() ; it != values.end();it++){
        (*it)->toJsonDoc(&json,minTime);
    }
    String buf;
    serializeJson(json,buf);
    return buf;
}
