#pragma once
#include <Arduino.h>
#include "GwApi.h"
#include <functional>
#include <vector>

#define MAX_PAGE_NUMBER 4
typedef std::vector<GwApi::BoatValue *> ValueList;
typedef struct{
  String pageName;
  //the values will always contain the user defined values first
  ValueList values;
} PageData;

typedef struct{
  double batteryVoltage = 0;
  double batteryCurrent = 0;
  double batteryPower = 0;
  double solarVoltage = 0;
  double solarCurrent = 0;
  double solarPower = 0;
  double generatorVoltage = 0;
  double generatorCurrent = 0;
  double generatorPower = 0;
  double airTemperature = 21.3;
  double airHumidity = 43.2;
  double airPressure = 1018.8;
  double onewireTemp1 = 0;
  double onewireTemp2 = 0;
  double onewireTemp3 = 0;
  double onewireTemp4 = 0;
  double onewireTemp5 = 0; 
  double onewireTemp6 = 0;
} SensorData;

typedef struct{
  GwApi::Status status;
  GwLog *logger=NULL;
  GwConfigHandler *config=NULL;
  SensorData data;
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
  String svalue;
  String unit;
} FormatedData;


// Formater for boat values
FormatedData formatValue(GwApi::BoatValue *value, CommonData &commondata);
