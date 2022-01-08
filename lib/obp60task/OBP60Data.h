#ifndef _OBP60Data_H
#define _OBP60Data_H

#include <Arduino.h>

typedef struct{               // Sub structure for bus data
  float fvalue = 0;           // Float value
  char svalue[16] = "";       // Char value
  char unit[8] = "";          // Unit
  bool valid = 0;             // Valid flag
} dataContainer;

typedef struct{
  // OBP60 Settings
  char dateformat[3] = "GB";
  int timezone = 0;
  float draft = 0;
  float fueltank = 0;
  float fuelconsumption = 0;
  float watertank = 0;
  float wastetank = 0;
  float batvoltage = 0;
  char battype[16] = "Pb";
  float batcapacity = 0;
  // OBP60 Hardware
  bool gps = false;
  bool bme280 = false;
  bool onewire = false;
  char powermode[16] = "Max Power";
  bool simulation = false;
  // OBP60 Display
  char displaymode[16] = "Logo + QR Code";
  bool statusline = true;
  bool refresh = false;
  char backlight[16] = "Control by Key";
  char flashled[16] = "Off";
  // OBP60 Buzzer
  bool buzerror = false;
  bool buzgps = false;
  bool buzlimits = false;
  char buzmode[16] = "Off";
  int buzpower = 0;
  // OBP60 Pages
  int numpages = 1;
  // Bus data
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