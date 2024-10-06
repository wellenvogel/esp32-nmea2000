#include "GwBoatData.h"
#include <GwJsonDocument.h>
#include <ArduinoJson/Json/TextFormatter.hpp>
#include "GWConfig.h"
#define GWTYPE_DOUBLE 1
#define GWTYPE_UINT32 2
#define GWTYPE_UINT16 3
#define GWTYPE_INT16 4
#define GWTYPE_USER 100

class GwBoatItemTypes
{
public:
    static int getType(const uint32_t &x) { return GWTYPE_UINT32; }
    static int getType(const uint16_t &x) { return GWTYPE_UINT16; }
    static int getType(const int16_t &x) { return GWTYPE_INT16; }
    static int getType(const double &x) { return GWTYPE_DOUBLE; }
    static int getType(const GwSatInfoList &x) { return GWTYPE_USER + 1; }
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
    this->lastUpdateSource = -1;
    this->toType=TOType::user;
}
GwBoatItemBase::GwBoatItemBase(String name, String format, GwBoatItemBase::TOType toType)
{
    lastSet = 0;
    this->invalidTime = INVALID_TIME;
    this->toType=toType;
    this->name = name;
    this->format = format;
    this->type = 0;
    this->lastUpdateSource = -1;
    this->toType=TOType::user;
}
void GwBoatItemBase::setInvalidTime(unsigned long it, bool force){
    if (toType != TOType::user || force ){
        invalidTime=it;
    }
}
size_t GwBoatItemBase::getJsonSize() { return JSON_OBJECT_SIZE(10); }
#define STRING_SIZE 40
GwBoatItemBase::StringWriter::StringWriter()
{
    buffer = new uint8_t[STRING_SIZE];
    wp = buffer;
    bufferSize = STRING_SIZE;
    buffer[0] = 0;
};
const char *GwBoatItemBase::StringWriter::c_str() const
{
    return (const char *)buffer;
}
int GwBoatItemBase::StringWriter::getSize() const
{
    int rt=wp - buffer;
    if (rt >= baseOffset) rt-=baseOffset;
    return rt;
}
void GwBoatItemBase::StringWriter::setBase()
{
    baseOffset = wp - buffer;
}
bool GwBoatItemBase::StringWriter::baseFilled()
{
    return baseOffset != 0;
}
void GwBoatItemBase::StringWriter::reset()
{
    wp = buffer + baseOffset;
    *wp = 0;
}
void GwBoatItemBase::StringWriter::ensure(size_t size)
{
    size_t fill = wp - buffer;
    size_t newSize = bufferSize;
    while ((fill + size) >= (newSize - 1))
    {
        newSize += STRING_SIZE;
    }
    if (newSize != bufferSize)
    {
        uint8_t *newBuffer = new uint8_t[newSize];
        memcpy(newBuffer, buffer, fill);
        newBuffer[fill] = 0;
        delete buffer;
        buffer = newBuffer;
        wp = newBuffer + fill;
        bufferSize = newSize;
    }
}
size_t GwBoatItemBase::StringWriter::write(uint8_t c)
{
    ensure(1);
    *wp = c;
    wp++;
    *wp = 0;
    return 1;
}
size_t GwBoatItemBase::StringWriter::write(const uint8_t *s, size_t n)
{
    ensure(n);
    memcpy(wp, s, n);
    wp += n;
    *wp = 0;
    return n;
}
template <class T>
GwBoatItem<T>::GwBoatItem(String name, String formatInfo, unsigned long invalidTime, GwBoatItemMap *map) : GwBoatItemBase(name, formatInfo, invalidTime)
{
    T dummy;
    this->type = GwBoatItemTypes::getType(dummy);
    if (map)
    {
        (*map)[name] = this;
    }
}
template <class T>
GwBoatItem<T>::GwBoatItem(String name, String formatInfo, GwBoatItemBase::TOType toType, GwBoatItemMap *map) : GwBoatItemBase(name, formatInfo, toType)
{
    T dummy;
    this->type = GwBoatItemTypes::getType(dummy);
    if (map)
    {
        (*map)[name] = this;
    }
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
void GwBoatItem<T>::toJsonDoc(GwJsonDocument *doc, unsigned long minTime)
{
    JsonObject o = doc->createNestedObject(name);
    o[F("value")] = getData();
    o[F("update")] = minTime - lastSet;
    o[F("source")] = lastUpdateSource;
    o[F("valid")] = isValid(minTime);
    o[F("format")] = format;
}

class WriterWrapper
{
    GwBoatItemBase::StringWriter *writer = NULL;

public:
    WriterWrapper(GwBoatItemBase::StringWriter *w)
    {
        writer = w;
    }
    size_t write(uint8_t c)
    {
        if (!writer)
            return 0;
        return writer->write(c);
    }
    size_t write(const uint8_t *s, size_t n)
    {
        if (!writer)
            return 0;
        return writer->write(s, n);
    }
};
typedef ARDUINOJSON_NAMESPACE::TextFormatter<WriterWrapper> GwTextWriter;

static void writeToString(GwTextWriter *writer, const double &value)
{
    writer->writeFloat(value);
}
static void writeToString(GwTextWriter *writer, const uint16_t &value)
{
    writer->writeInteger(value);
}
static void writeToString(GwTextWriter *writer, const uint32_t &value)
{
    writer->writeInteger(value);
}
static void writeToString(GwTextWriter *writer, const int16_t &value)
{
    writer->writeInteger(value);
}
static void writeToString(GwTextWriter *writer, GwSatInfoList &value)
{
    writer->writeInteger(value.getNumSats());
}

template <class T>
void GwBoatItem<T>::fillString()
{
    bool valid = isValid();
    if (writer.getSize() && (valid == lastStringValid))
        return;
    lastStringValid = valid;
    writer.reset();
    WriterWrapper wrapper(&writer);
    GwTextWriter stringWriter(wrapper);
    if (!writer.baseFilled())
    {
        stringWriter.writeRaw(name.c_str());
        stringWriter.writeChar(',');
        stringWriter.writeRaw(format.c_str());
        stringWriter.writeChar(',');
        writer.setBase();
    }
    stringWriter.writeInteger(valid ? 1 : 0);
    stringWriter.writeChar(',');
    stringWriter.writeInteger(lastSet);
    stringWriter.writeChar(',');
    stringWriter.writeInteger(lastUpdateSource);
    stringWriter.writeChar(',');
    writeToString(&stringWriter, data);
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
                       return info.validTill < ts;
                   }),
               sats.end());
}
void GwSatInfoList::update(GwSatInfo entry, unsigned long validTill)
{
    entry.validTill = validTill;
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

GwBoatDataSatList::GwBoatDataSatList(String name, String formatInfo, GwBoatItemBase::TOType toType, GwBoatItemMap *map) : GwBoatItem<GwSatInfoList>(name, formatInfo, toType, map) {}

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
    data.update(info,now+invalidTime);
    return true;
}
void GwBoatDataSatList::toJsonDoc(GwJsonDocument *doc, unsigned long minTime)
{
    data.houseKeeping();
    GwBoatItem<GwSatInfoList>::toJsonDoc(doc, minTime);
}

GwBoatData::GwBoatData(GwLog *logger, GwConfigHandler *cfg)
{
    this->logger = logger;
    for (auto &&it : values){
        unsigned long timeout=GwBoatItemBase::INVALID_TIME;
        switch(it.second->getToType()){
            case GwBoatItemBase::TOType::ais:
                timeout=cfg->getInt(GwConfigDefinitions::timoAis);
                break;
            case GwBoatItemBase::TOType::def:
                timeout=cfg->getInt(GwConfigDefinitions::timoDefault);
                break;
            case GwBoatItemBase::TOType::lng:
                timeout=cfg->getInt(GwConfigDefinitions::timoLong);
                break;
            case GwBoatItemBase::TOType::sensor:
                timeout=cfg->getInt(GwConfigDefinitions::timoSensor);
                break;
            case GwBoatItemBase::TOType::keep:
                timeout=0;
                break;
        }
        it.second->setInvalidTime(timeout);
    }

}
GwBoatData::~GwBoatData()
{
    GwBoatItemBase::GwBoatItemMap::iterator it;
    for (it = values.begin(); it != values.end(); it++)
    {
        delete it->second;
    }
}

template <class T>
GwBoatItem<T> *GwBoatData::getOrCreate(T initial, GwBoatItemNameProvider *provider)
{
    String name = provider->getBoatItemName();
    auto it = values.find(name);
    if (it != values.end())
    {
        int expectedType = GwBoatItemTypes::getType(initial);
        if (expectedType != it->second->getCurrentType())
        {
            LOG_DEBUG(GwLog::DEBUG, "invalid type for boat item %s, expected %d, got %d",
                      name.c_str(), expectedType, it->second->getCurrentType());
            return NULL;
        }
        return (GwBoatItem<T> *)(it->second);
    }
    GwBoatItem<T> *rt = new GwBoatItem<T>(name,
                                          provider->getBoatItemFormat(),
                                          provider->getInvalidTime(),
                                          &values);
    rt->update(initial);
    LOG_DEBUG(GwLog::LOG, "creating boatItem %s, type %d",
              name.c_str(), rt->getCurrentType());
    return rt;
}
template <class T>
bool GwBoatData::update(const T value, int source, GwBoatItemNameProvider *provider)
{
    GwBoatItem<T> *item = getOrCreate(value, provider);
    if (!item)
        return false;
    return item->update(value, source);
}
template bool GwBoatData::update<double>(double value, int source, GwBoatItemNameProvider *provider);
template <class T>
T GwBoatData::getDataWithDefault(T defaultv, GwBoatItemNameProvider *provider)
{
    auto it = values.find(provider->getBoatItemName());
    if (it == values.end())
        return defaultv;
    int expectedType = GwBoatItemTypes::getType(defaultv);
    if (expectedType != it->second->getCurrentType())
        return defaultv;
    return ((GwBoatItem<T> *)(it->second))->getDataWithDefault(defaultv);
}
template double GwBoatData::getDataWithDefault<double>(double defaultv, GwBoatItemNameProvider *provider);
String GwBoatData::toJson() const
{
    unsigned long minTime = millis();
    GwBoatItemBase::GwBoatItemMap::const_iterator it;
    size_t count = 0;
    size_t elementSizes = 0;
    for (it = values.begin(); it != values.end(); it++)
    {
        count++;
        elementSizes += it->second->getJsonSize();
    }
    int sz = JSON_OBJECT_SIZE(count) + elementSizes + 10;
    LOG_DEBUG(GwLog::DEBUG, "size for boatData: %d", sz);
    GwJsonDocument json(sz);
    for (it = values.begin(); it != values.end(); it++)
    {
        it->second->toJsonDoc(&json, minTime);
    }
    String buf;
    serializeJson(json, buf);
    return buf;
}
String GwBoatData::toString()
{
    String rt;
    rt.reserve(values.size() * 80);
    for (auto it = values.begin(); it != values.end(); it++)
    {
        rt += it->second->getDataString();
        rt += "\n";
    }
    return rt;
}
bool GwBoatData::isValid(String name)
{
    auto it = values.find(name);
    if (it == values.end())
        return false;
    return it->second->isValid();
}
GwBoatItemBase *GwBoatData::getBase(String name)
{
    auto it = values.find(name);
    if (it == values.end())
        return NULL;
    return it->second;
}
double GwBoatData::getDoubleValue(String name, double defaultv)
{
    auto it = values.find(name);
    if (it == values.end())
        return defaultv;
    if (!it->second->isValid())
        return defaultv;
    return it->second->getDoubleValue();
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
double formatDegToRad(double deg)
{
    return deg / 180.0 * M_PI;
}
double formatWind(double cv)
{
    double rt = formatCourse(cv);
    if (rt > 180)
        rt = -1 * (360 - rt);
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

bool convertToJson(const GwSatInfoList &si, JsonVariant &variant)
{
    return variant.set(si.getNumSats());
}

#ifdef _UNDEF
#include <ArduinoJson/Json/TextFormatter.hpp>

class XWriter
{
public:
    void write(uint8_t c)
    {
    }

    void write(const uint8_t *s, size_t n)
    {
    }
};
static XWriter xwriter;
ARDUINOJSON_NAMESPACE::TextFormatter<XWriter> testWriter(xwriter);
#endif