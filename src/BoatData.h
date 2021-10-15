#ifndef _BoatData_H_
#define _BoatData_H_

struct tBoatData {
  unsigned long DaysSince1970;   // Days since 1970-01-01
  
  double Heading,SOG,COG,STW,Variation,AWS,TWS,MaxAws,MaxTws,AWA,TWA,AWD,TWD,TripLog,Log,RudderPosition,WaterTemperature,
         WaterDepth, GPSTime,// Secs since midnight,
         Latitude, Longitude, Altitude;
  
public:
  tBoatData() {
    Heading=0;
    Latitude=0;
    Longitude=0;
    SOG=0;
    COG=0;
    STW=0;
    AWS=0;
    TWS=0;
    MaxAws=0;
    MaxTws=0;
    AWA=0;
    TWA=0;
    TWD=0;
    TripLog=0;
    Log=0;
    RudderPosition=0;
    WaterTemperature=0;
    WaterDepth=0;
    Variation=0;
    Altitude=0;
    GPSTime=0;
    DaysSince1970=0;    
  };
};

#endif // _BoatData_H_
