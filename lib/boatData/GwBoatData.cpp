#include "GwBoatData.h"

GwBoatData::GwBoatData(GwLog *logger){
    this->logger=logger;
}
GwBoatData::~GwBoatData(){
    GwBoatItemBase::GwBoatItemMap::iterator it;
    for (it=values.begin() ; it != values.end();it++){
        delete it->second;
    }
}

template<class T> GwBoatItem<T> *GwBoatData::getOrCreate(T initial, GwBoatItemNameProvider *provider)
{
    String name=provider->getBoatItemName();
    auto it=values.find(name);
    if (it != values.end()) {
        int expectedType=GwBoatItemTypes::getType(initial);
        if (expectedType != it->second->getCurrentType()){
            return NULL;
        }
        return (GwBoatItem<T>*)(it->second);
    }
    GwBoatItem<T> *rt=new GwBoatItem<T>(GwBoatItemTypes::getType(initial), name,
        provider->getBoatItemFormat(),
        provider->getInvalidTime(),
        &values);
    rt->update(initial);
    return rt;
}
template<class T> bool GwBoatData::update(T value,int source,GwBoatItemNameProvider *provider){
    GwBoatItem<T> *item=getOrCreate(value,provider);
    if (! item) return false;
    return item->update(value,source);
}
template bool GwBoatData::update<double>(double value,int source,GwBoatItemNameProvider *provider);
template<class T> T GwBoatData::getDataWithDefault(T defaultv, GwBoatItemNameProvider *provider){
    auto it=values.find(provider->getBoatItemName());
    if (it == values.end()) return defaultv;
    int expectedType=GwBoatItemTypes::getType(defaultv);
    if (expectedType != it->second->getCurrentType()) return defaultv;
    return ((GwBoatItem<T> *)(it->second))->getDataWithDefault(defaultv);
}
template double GwBoatData::getDataWithDefault<double>(double defaultv, GwBoatItemNameProvider *provider);
String GwBoatData::toJson() const {
    unsigned long minTime=millis();
    GwBoatItemBase::GwBoatItemMap::const_iterator it;
    size_t count=0;
    size_t elementSizes=0;
    for (it=values.begin() ; it != values.end();it++){
        count++;
        elementSizes+=it->second->getJsonSize();
    }
    DynamicJsonDocument json(JSON_OBJECT_SIZE(count)+elementSizes+10);
    for (it=values.begin() ; it != values.end();it++){
        it->second->toJsonDoc(&json,minTime);
    }
    String buf;
    serializeJson(json,buf);
    return buf;
}

double formatCourse(double cv)
{
    double rt = cv * 180.0 / M_PI;
    if (rt > 360)
        rt -= 360;
    if (rt < 0)
        rt += 360;
    return rt;
}
double formatDegToRad(double deg){
    return deg/180.0 * M_PI;
}
double formatWind(double cv)
{
    double rt = formatCourse(cv);
    if (rt > 180)
        rt = 180 - rt;
    return rt;
}
double formatKnots(double cv)
{
    return cv * 3600.0 / 1852.0;
}

uint32_t mtr2nm(uint32_t m)
{
    return m / 1852;
}
double mtr2nm(double m)
{
    return m / 1852.0;
}

bool convertToJson(const GwSatInfoList &si,JsonVariant &variant){
    return variant.set(si.getNumSats());
}