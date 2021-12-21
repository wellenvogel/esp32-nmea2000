#ifndef _OBP60Data_H
#define _OBP60Data_H

#include <Arduino.h>

typedef struct{
  float fvalue = 0;           // Float value
  char svalue[16] = "";       // Char value
  char unit[8] = "";          // Unit
  int valid = 0;              // Valid flag
} dataContainer;

typedef struct{
  bool simulation = false;    // Simulate boat data
  bool statusline = true;
  char dateformat[3] = "GB";
  int timezone = 0;
  bool refresh = false;
  dataContainer AWA;
  dataContainer AWD;
  dataContainer AWS;
  dataContainer Altitude;
  dataContainer BTW;
  dataContainer COG;
  dataContainer DTW;
  dataContainer Date;
  dataContainer DepthTransducer;
  dataContainer Deviation;
  dataContainer HDOP;
  dataContainer Heading;
  dataContainer Latitude;
  dataContainer Log;
  dataContainer Longitude;
  dataContainer MagneticHeading;
  dataContainer MaxAws;
  dataContainer MaxTws;
  dataContainer PDOP;
  dataContainer ROT;
  dataContainer RudderPosition;
  dataContainer SOG;
  dataContainer STW;
  dataContainer SatInfo;
  dataContainer Time;
  dataContainer TWD;
  dataContainer TWS;
  dataContainer Timezone;
  dataContainer TripLog;
  dataContainer VDOP;
  dataContainer Variation;
  dataContainer WPLatitude;
  dataContainer WPLongitude;
  dataContainer WaterDepth;
  dataContainer WaterTemperature;
  dataContainer XTE;
} busData;

busData busInfo;

#endif