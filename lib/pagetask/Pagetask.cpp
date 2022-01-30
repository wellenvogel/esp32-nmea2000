#include "Pagetask.h"


#ifdef BOARD_PAGETASK
#include "Pagedata.h"


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

class BoatValueList{
    public:
    static const int MAXVALUES=100;
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
};

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

class PageStruct{
    public:
        Page *page=NULL;
        PageData parameters;
        PageDescription *description=NULL;
};

/**
 * this function will add all the pages we know to the pagelist
 * each page should have defined a registerXXXPage variable of type
 * PageData that describes what it needs
 */
void registerAllPages(PageList &list){
    //the next line says that this variable is defined somewhere else
    //in our case in a separate C++ source file
    //this way this separate source file can be compiled by it's own
    //and has no access to any of our data except the one that we
    //give as a parameter to the page function
    extern PageDescription registerOneValuePage;
    //we add the variable to our list
    list.add(&registerOneValuePage);
    extern PageDescription registerTwoValuesPage;
    list.add(&registerTwoValuesPage);
    extern PageDescription registerThreeValuesPage;
    list.add(&registerThreeValuesPage);
    extern PageDescription registerForValuesPage;
    list.add(&registerForValuesPage);
    extern PageDescription registerApparentWindPage;
    list.add(&registerApparentWindPage);
}

void pageTask(GwApi *api){
    GwLog *logger=api->getLogger();
    GwConfigHandler *config=api->getConfig();
    PageList allPages;
    registerAllPages(allPages);
    LOG_DEBUG(GwLog::LOG,"page task started");
    for (auto it=allPages.pages.begin();it != allPages.pages.end();it++){
        LOG_DEBUG(GwLog::LOG,"found registered page %s",(*it)->pageName.c_str());
    }
    int numPages=1;
    PageStruct pages[MAX_PAGE_NUMBER];
    CommonData commonData;
    commonData.logger=logger;
    BoatValueList boatValues; //all the boat values for the api query
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
       PageDescription *description=allPages.find(pageType);
       if (description == NULL){
           LOG_DEBUG(GwLog::ERROR,"page description for %s not found",pageType.c_str());
           continue;
       }
       pages[i].description=description;
       pages[i].page=description->creator(commonData);
       pages[i].parameters.pageName=pageType;
       LOG_DEBUG(GwLog::DEBUG,"found page %s for number %d",pageType.c_str(),i);
       //fill in all the user defined parameters
       for (int uid=0;uid<description->userParam;uid++){
           String cfgName=prefix+String("value")+String(uid+1);
           GwApi::BoatValue *value=boatValues.findValueOrCreate(config->getString(cfgName,""));
           LOG_DEBUG(GwLog::DEBUG,"add user input cfg=%s,value=%s for page %d",
                cfgName.c_str(),
                value->getName().c_str(),
                i
           );
           pages[i].parameters.values.push_back(value);
       }
       //now add the predefined values
       for (auto it=description->fixedParam.begin();it != description->fixedParam.end();it++){
            GwApi::BoatValue *value=boatValues.findValueOrCreate(*it);
            LOG_DEBUG(GwLog::DEBUG,"added fixed value %s to page %d",value->getName().c_str(),i);
            pages[i].parameters.values.push_back(value); 
       }
    }
    //now we have prepared the page data
    //we start a separate task that will fetch our keys...
    QueueHandle_t keyboardQueue=xQueueCreate(10,sizeof(int));
    xTaskCreate(keyboardTask,"keyboard",2000,&keyboardQueue,0,NULL);


    //loop
    LOG_DEBUG(GwLog::LOG,"pagetask: start mainloop");
    int pageNumber=0;
    int lastPage=pageNumber;
    while (true){
        delay(1000);
        //check if there is a keyboard message
        int keyboardMessage=-1;
        if (xQueueReceive(keyboardQueue,&keyboardMessage,0)){
            LOG_DEBUG(GwLog::LOG,"new page from keyboard %d",keyboardMessage);
            if (keyboardMessage >= 0 && keyboardMessage < numPages){
                pageNumber=keyboardMessage;
            }
            LOG_DEBUG(GwLog::LOG,"set pagenumber to %d",pageNumber);
        }
        //refresh data from api
        api->getBoatDataValues(boatValues.numValues,boatValues.allBoatValues);
        api->getStatus(commonData.status);

        //handle the pag
        if (pages[pageNumber].description && pages[pageNumber].description->header){

        //build some header and footer using commonData
        }
        //....
        //call the particular page
        Page *currentPage=pages[pageNumber].page;
        if (currentPage == NULL){
            LOG_DEBUG(GwLog::ERROR,"page number %d not found",pageNumber);
        }
        else{
            if (lastPage != pageNumber){
                currentPage->displayNew(commonData,pages[pageNumber].parameters);
                lastPage=pageNumber;
            }
            //call the page code
            LOG_DEBUG(GwLog::DEBUG,"calling page %d type %s");
            currentPage->display(commonData,pages[pageNumber].parameters);
        }

    }
    vTaskDelete(NULL);

}

#endif