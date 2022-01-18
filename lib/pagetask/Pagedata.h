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
} CommonData;



typedef std::function<void *(CommonData &, PageData &)> PageFunction;
typedef std::vector<String> StringList;
class PageDescription;
void registerPage(PageDescription *p);
class PageDescription{
    public:
        String pageName;
        int userParam=0;
        StringList fixedParam;
        PageFunction function;
        PageDescription(String name, PageFunction function,int userParam,StringList fixedParam){
            this->pageName=pageName;
            this->userParam=userParam;
            this->fixedParam=fixedParam;
            this->function=function;
            registerPage(this);
        }
};
