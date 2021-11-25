#include "GwBoatData.h"
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
        static int getType(const GwSatInfoList &x){ return GWTYPE_USER+1;}
};

bool GwBoatItemBase::isValid(unsigned long now) const
{
    if (lastSet == 0)
        return false;
    if (invalidTime == 0)
        return true;
    if (now == 0)
        now = millis();
    return (lastSet + invalidTime) >= now;
}
GwBoatItemBase::GwBoatItemBase(String name, String format, unsigned long invalidTime)
{
    lastSet = 0;
    this->invalidTime = invalidTime;
    this->name = name;
    this->format = format;
    this->type = 0;
}

template<class T> GwBoatItem<T>::GwBoatItem(String name,String formatInfo,unsigned long invalidTime,GwBoatItemMap *map):
    GwBoatItemBase(name,formatInfo,invalidTime){
            T dummy;
            this->type=GwBoatItemTypes::getType(dummy);
            if (map){
                (*map)[name]=this;
            }
            lastUpdateSource=-1;
}

template <class T>
bool GwBoatItem<T>::update(T nv, int source)
{
    unsigned long now = millis();
    if (isValid(now))
    {
        //priority handling
        //sources with lower ids will win
        //and we will not overwrite their value
        if (lastUpdateSource < source && lastUpdateSource >= 0)
        {
            return false;
        }
    }
    data = nv;
    lastUpdateSource = source;
    uls(now);
    return true;
}
template <class T>
bool GwBoatItem<T>::updateMax(T nv, int sourceId)
{
    unsigned long now = millis();
    if (!isValid(now))
    {
        return update(nv, sourceId);
    }
    if (getData() < nv)
    {
        data = nv;
        lastUpdateSource = sourceId;
        uls(now);
        return true;
    }
    return false;
}
template <class T>
void GwBoatItem<T>::toJsonDoc(JsonDocument *doc, unsigned long minTime)
{
    JsonObject o = doc->createNestedObject(name);
    o[F("value")] = getData();
    o[F("update")] = minTime - lastSet;
    o[F("source")] = lastUpdateSource;
    o[F("valid")] = isValid(minTime);
    o[F("format")] = format;
}
template class GwBoatItem<double>;
template class GwBoatItem<uint32_t>;
template class GwBoatItem<uint16_t>;
template class GwBoatItem<int16_t>;
void GwSatInfoList::houseKeeping(unsigned long ts)
{
    if (ts == 0)
        ts = millis();
    sats.erase(std::remove_if(
                   sats.begin(),
                   sats.end(),
                   [ts, this](const GwSatInfo &info)
                   {
                       return (info.timestamp + lifeTime) < ts;
                   }),
               sats.end());
}
void GwSatInfoList::update(GwSatInfo entry)
{
    unsigned long now = millis();
    entry.timestamp = now;
    for (auto it = sats.begin(); it != sats.end(); it++)
    {
        if (it->PRN == entry.PRN)
        {
            *it = entry;
            houseKeeping();
            return;
        }
    }
    houseKeeping();
    sats.push_back(entry);
}

GwBoatDataSatList::GwBoatDataSatList(String name, String formatInfo, unsigned long invalidTime , GwBoatItemMap *map) : 
        GwBoatItem<GwSatInfoList>(name, formatInfo, invalidTime, map) {}

bool GwBoatDataSatList::update(GwSatInfo info, int source)
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
void GwBoatDataSatList::toJsonDoc(JsonDocument *doc, unsigned long minTime)
{
    data.houseKeeping();
    GwBoatItem<GwSatInfoList>::toJsonDoc(doc, minTime);
}

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
            LOG_DEBUG(GwLog::DEBUG,"invalid type for boat item %s, expected %d, got %d",
                name.c_str(),expectedType,it->second->getCurrentType());
            return NULL;
        }
        return (GwBoatItem<T>*)(it->second);
    }
    GwBoatItem<T> *rt=new GwBoatItem<T>(name,
        provider->getBoatItemFormat(),
        provider->getInvalidTime(),
        &values);
    rt->update(initial);
    LOG_DEBUG(GwLog::LOG,"creating boatItem %s, type %d",
        name.c_str(),rt->getCurrentType());
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
    int sz=JSON_OBJECT_SIZE(count)+elementSizes+10;
    LOG_DEBUG(GwLog::DEBUG,"size for boatData: %d",sz);
    DynamicJsonDocument json(sz);
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

#ifdef _UNDEF
#include <ArduinoJson/Json/TextFormatter.hpp>

class XWriter{
    public:
     void write(uint8_t c) {
    }

  void write(const uint8_t* s, size_t n) {
  }   
};
static XWriter xwriter;
ARDUINOJSON_NAMESPACE::TextFormatter<XWriter> testWriter(xwriter);
#endif