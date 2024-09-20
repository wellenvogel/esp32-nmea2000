#pragma once
#include <Arduino.h>
#include "GwApi.h"
#include <functional>
#include <vector>

typedef std::vector<GwApi::BoatValue *> ValueList;
typedef struct{
  String pageName;
  //the values will always contain the user defined values first
  ValueList values;
} PageData;

// Sensor data structure (only for extended sensors, not for NMEA bus sensors)
typedef struct{
  int actpage = 0;
  int maxpage = 0;
  double batteryVoltage = 0;
  double batteryCurrent = 0;
  double batteryPower = 0;
  double batteryVoltage10 = 0;  // Sliding average over 10 values
  double batteryCurrent10 = 0;
  double batteryPower10 = 0;
  double batteryVoltage60 = 0;  // Sliding average over 60 values
  double batteryCurrent60 = 0;
  double batteryPower60 = 0;
  double batteryVoltage300 = 0; // Sliding average over 300 values
  double batteryCurrent300 = 0;
  double batteryPower300 = 0;
  double solarVoltage = 0;
  double solarCurrent = 0;
  double solarPower = 0;
  double generatorVoltage = 0;
  double generatorCurrent = 0;
  double generatorPower = 0;
  double airTemperature = 0;
  double airHumidity = 0;
  double airPressure = 0;
  double onewireTemp[8] = {0,0,0,0,0,0,0,0};
  double rotationAngle = 0;       // Rotation angle in radiant
  bool validRotAngle = false;     // Valid flag magnet present for rotation sensor
  int rtcYear = 0;  // UTC time
  int rtcMonth = 0;
  int rtcDay = 0;
  int rtcHour = 0;
  int rtcMinute = 0;
  int rtcSecond = 0;
  int sunsetHour = 0;
  int sunsetMinute = 0;
  int sunriseHour = 0;
  int sunriseMinute = 0;
  bool sunDown = true;
} SensorData;

typedef struct{
  int sunsetHour = 0;
  int sunsetMinute = 0;
  int sunriseHour = 0;
  int sunriseMinute = 0;
  bool sunDown = true;
} SunData;

typedef struct{
  GwApi::Status status;
  GwLog *logger=NULL;
  GwConfigHandler *config=NULL;
  SensorData data;
  SunData sundata;
  GwApi::BoatValue *time=NULL;
  GwApi::BoatValue *date=NULL;
} CommonData;

//a base class that all pages must inherit from
class Page{
  public:
    virtual void displayPage(CommonData &commonData, PageData &pageData)=0;
    virtual void displayNew(CommonData &commonData, PageData &pageData){}
    //return -1 if handled by the page
    virtual int handleKey(int key){return key;}
};

typedef std::function<Page* (CommonData &)> PageFunction;
typedef std::vector<String> StringList;

/**
 * a class that describes a page
 * it contains the name (type)
 * the number of expected user defined boat Values
 * and a list of boatValue names that are fixed
 * for each page you define a variable of this type
 * and add this to registerAllPages in the obp60task
 */
class PageDescription{
    public:
        String pageName;
        int userParam=0;
        StringList fixedParam;
        PageFunction creator;
        bool header=true;
        PageDescription(String name, PageFunction creator,int userParam,StringList fixedParam,bool header=true){
            this->pageName=name;
            this->userParam=userParam;
            this->fixedParam=fixedParam;
            this->creator=creator;
            this->header=header;
        }
        PageDescription(String name, PageFunction creator,int userParam,bool header=true){
            this->pageName=name;
            this->userParam=userParam;
            this->creator=creator;
            this->header=header;
        }
};

// Structure for formated boat values
typedef struct{
  double value;
  String svalue;
  String unit;
} FormatedData;


// Formater for boat values
FormatedData formatValue(GwApi::BoatValue *value, CommonData &commondata);
