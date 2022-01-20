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

}OutputData;

typedef struct{
  String distanceformat="m";
  String lengthformat="m";
  //...
  OutputData output;
  GwApi::Status status;
  GwLog *logger=NULL;
} CommonData;



typedef std::function<void (CommonData &, PageData &)> PageFunction;
typedef std::vector<String> StringList;

/**
 * a class that describes a page
 * it contains the name (type)
 * the number of expected user defined boat Values
 * and a list of boatValue names that are fixed
 * for each page you define a variable of this type
 * and add this to registerAllPages in the Pagetask
 */
class PageDescription{
    public:
        String pageName;
        int userParam=0;
        StringList fixedParam;
        PageFunction function;
        PageDescription(String name, PageFunction function,int userParam,StringList fixedParam){
            this->pageName=name;
            this->userParam=userParam;
            this->fixedParam=fixedParam;
            this->function=function;
        }
        PageDescription(String name, PageFunction function,int userParam){
            this->pageName=name;
            this->userParam=userParam;
            this->function=function;
        }
};
