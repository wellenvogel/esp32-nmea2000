#include "Pagetask.h"
#include "Pagedata.h"


//#ifdef BOARD_PAGETASK

//include all the pages here
#include "OneValuePage.hpp"

void pageInit(GwApi *param){
    param->getLogger()->logDebug(GwLog::LOG,"page init running");
}

void keyboardTask(void *param){
    QueueHandle_t *queue=(QueueHandle_t*)param;
    int page=0;
    while (true){
        //send a key event 
        xQueueSend(*queue, &page, 0);
        delay(10000);
        page+=1;
        if (page>=MAX_PAGE_NUMBER) page=0;
    }
    vTaskDelete(NULL);
}

#define MAXVALUES 100
//we create a list containing all our BoatValues
//this is the list we later use to let the api fill all the values
//additionally we put the necessary values into the paga data - see below
GwApi::BoatValue *allBoatValues[MAXVALUES];
int numValues=0;
bool addValueToList(GwApi::BoatValue *v){
    for (int i=0;i<numValues;i++){
        if (allBoatValues[i] == v){
            //already in list...
            return true;
        }
    }
    if (numValues >= MAXVALUES) return false;
    allBoatValues[numValues]=v;
    numValues++;
    return true;
}
//helper to ensure that each BoatValue is only queried once
GwApi::BoatValue *findValueOrCreate(String name){
    for (int i=0;i<numValues;i++){
        if (allBoatValues[i]->getName() == name) {
            return allBoatValues[i];
        }
    }
    GwApi::BoatValue *rt=new GwApi::BoatValue(name);
    addValueToList(rt);
    return rt;
}

//we want to have a list that has all our page definitions
//this way each page can easily be added here
//needs some minor tricks for the safe static initialization
typedef std::vector<PageDescription*> Pages;
//the page list class
class PageList{
    public:
        Pages pages;
        void add(PageDescription *p){
            pages.push_back(p);
        }
        PageDescription *find(String name){
            for (auto it=pages.begin();it != pages.end();it++){
                if ((*it)->pageName == name){
                    return *it;
                }
            }
            return NULL;
        }
};

//a function to get a static instance of this class
PageList & pageList(){
    static PageList instance;
    return instance;
}
void registerPage(PageDescription *p){
    pageList().add(p);
}
void pageTask(GwApi *api){
    GwLog *logger=api->getLogger();
    GwConfigHandler *config=api->getConfig();
    LOG_DEBUG(GwLog::LOG,"page task started");
    for (auto it=pageList().pages.begin();it != pageList().pages.end();it++){
        LOG_DEBUG(GwLog::LOG,"found registered page %s",(*it)->pageName.c_str());
    }
    int numPages=1;
    PageData pages[MAX_PAGE_NUMBER];
    CommonData commonData;
    commonData.logger=logger;
    //commonData.distanceformat=config->getString(xxx);
    //add all necessary data to common data

    //fill the page data from config
    numPages=config->getInt(config->visiblePages,1);
    if (numPages < 1) numPages=1;
    if (numPages >= MAX_PAGE_NUMBER) numPages=MAX_PAGE_NUMBER;
    String configPrefix="page";
    for (int i=0;i< numPages;i++){
       String prefix=configPrefix+String(i+1); //e.g. page1
       String configName=prefix+String("type");
       LOG_DEBUG(GwLog::DEBUG,"asking for page config %s",configName.c_str());
       String pageType=config->getString(configName,"");
       PageDescription *description=pageList().find(pageType);
       if (description == NULL){
           LOG_DEBUG(GwLog::ERROR,"page description for %s not found",pageType.c_str());
           continue;
       }
       pages[i].pageName=pageType;
       LOG_DEBUG(GwLog::DEBUG,"found page %s for number %d",pageType.c_str(),i);
       //fill in all the user defined parameters
       for (int uid=0;uid<description->userParam;uid++){
           String cfgName=prefix+String("value")+String(uid+1);
           GwApi::BoatValue *value=findValueOrCreate(config->getString(cfgName,""));
           LOG_DEBUG(GwLog::DEBUG,"add user input cfg=%s,value=%s for page %d",
                cfgName.c_str(),
                value->getName().c_str(),
                i
           );
           pages[i].values.push_back(value);
       }
       //now add the predefined values
       for (auto it=description->fixedParam.begin();it != description->fixedParam.end();it++){
            GwApi::BoatValue *value=findValueOrCreate(*it);
            LOG_DEBUG(GwLog::DEBUG,"added fixed value %s to page %d",value->getName().c_str(),i);
            pages[i].values.push_back(value); 
       }
    }
    //now we have prepared the page data
    //we start a separate task that will fetch our keys...
    QueueHandle_t keyboardQueue=xQueueCreate(10,sizeof(int));
    xTaskCreate(keyboardTask,"keyboard",2000,&keyboardQueue,0,NULL);


    //loop
    LOG_DEBUG(GwLog::LOG,"pagetask: start mainloop");
    int currentPage=0;
    while (true){
        delay(1000);
        //check if there is a keyboard message
        int keyboardMessage=-1;
        if (xQueueReceive(keyboardQueue,&keyboardMessage,0)){
            LOG_DEBUG(GwLog::LOG,"new page %d",keyboardMessage);
            if (keyboardMessage >= 0 && keyboardMessage < MAX_PAGE_NUMBER){
                currentPage=keyboardMessage;
            }
        }
        //refresh data from api
        api->getBoatDataValues(numValues,allBoatValues);
        api->getStatus(commonData.status);

        //handle the page
        //build some header and footer using commonData
        //....
        //call the particular page
        String currentType=pages[currentPage].pageName;
        PageDescription *description=pageList().find(currentType);
        if (description == NULL){
            LOG_DEBUG(GwLog::ERROR,"page number %d, type %s not found",currentPage, currentType.c_str());
        }
        else{
            //call the page code
            LOG_DEBUG(GwLog::DEBUG,"calling page %d type %s",currentPage,description->pageName.c_str());
            description->function(commonData,pages[currentPage]);
        }

    }
    vTaskDelete(NULL);

}

//#endif