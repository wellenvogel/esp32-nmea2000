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
    return m / 1852;
}

double kelvinToC(double v)
{
    return v - 273.15;
}
double formatLatitude(double v){
    return v;
}
double formatLongitude(double v){
    return v;
}
double formatFixed0(double v){
    return v;
}
double formatDepth(double v){
    return v;
}
uint32_t formatFixed0(uint32_t v){
    return v;
}