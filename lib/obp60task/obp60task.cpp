#ifdef BOARD_NODEMCU32S_OBP60
#include "obp60task.h"
#include "Pagedata.h"
#include "OBP60Hardware.h"              // PIN definitions
#include <Ticker.h>                     // Timer Lib for timer interrupts
#include <Wire.h>                       // I2C connections
#include <MCP23017.h>                   // MCP23017 extension Port
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <GxEPD.h>                      // GxEPD lib for E-Ink displays
#include <GxGDEW042T2/GxGDEW042T2.h>    // 4.2" Waveshare S/W 300 x 400 pixel
#include <GxIO/GxIO_SPI/GxIO_SPI.h>     // GxEPD lip for SPI display communikation
#include <GxIO/GxIO.h>                  // GxEPD lip for SPI
#include "OBP60ExtensionPort.h"         // Functions lib for extension board

// True type character sets
#include "Ubuntu_Bold8pt7b.h"
#include "Ubuntu_Bold20pt7b.h"
#include "Ubuntu_Bold32pt7b.h"
#include "DSEG7Classic-BoldItalic16pt7b.h"
#include "DSEG7Classic-BoldItalic42pt7b.h"
#include "DSEG7Classic-BoldItalic60pt7b.h"

// Pictures
//#include GxEPD_BitmapExamples         // Example picture
#include "MFD_OBP60_400x300_sw.h"       // MFD with logo
#include "Logo_OBP_400x300_sw.h"        // OBP Logo

tNMEA0183Msg NMEA0183Msg;
tNMEA0183 NMEA0183;

// Timer Interrupts for hardware functions
Ticker Timer1;  // Under voltage detection
Ticker Timer2;  // Binking flash LED

// Global vars
bool initComplete = false;      // Initialization complete
int taskRunCounter = 0;         // Task couter for loop section
bool gps_ready = false;         // GPS initialized and ready to use


// Hardware initialisation before start all services
//##################################################
void OBP60Init(GwApi *api){
    api->getLogger()->logDebug(GwLog::LOG,"obp60init running");
    // Define timer interrupts
    bool uvoltage = api->getConfig()->getConfigItem(api->getConfig()->underVoltage,true)->asBoolean();
    if(uvoltage == true){
        Timer1.attach_ms(1, underVoltageDetection);     // Maximum speed with 1ms
    }
    Timer2.attach_ms(500, blinkingFlashLED); 

    // Extension port MCP23017
    // Check I2C devices MCP23017
    Wire.begin(OBP_I2C_SDA, OBP_I2C_SCL);
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        api->getLogger()->logDebug(GwLog::ERROR,"MCP23017 not found, check wiring");
        initComplete = false;
    }
    else{ 
        // Start communication
        mcp.init();
        mcp.portMode(MCP23017Port::A, 0b00110000);  //Port A, 0 = out, 1 = in
        mcp.portMode(MCP23017Port::B, 0b11110000);  //Port B, 0 = out, 1 = in

        // Extension Port A set defaults
        setPortPin(OBP_DIGITAL_OUT1, false);    // PA0
        setPortPin(OBP_DIGITAL_OUT2, false);    // PA1
        setPortPin(OBP_FLASH_LED, false);       // PA2
        setPortPin(OBP_BACKLIGHT_LED, false);   // PA3
        setPortPin(OBP_POWER_50, true);         // PA6
        setPortPin(OBP_POWER_33, true);         // PA7

        // Extension Port B set defaults
        setPortPin(PB0, false);     // PB0
        setPortPin(PB1, false);     // PB1
        setPortPin(PB2, false);     // PB2
        setPortPin(PB3, false);     // PB3

        // Settings for 1Wire
        bool enable1Wire = api->getConfig()->getConfigItem(api->getConfig()->use1Wire,true)->asBoolean();
        if(enable1Wire == true){
            api->getLogger()->logDebug(GwLog::DEBUG,"1Wire Mode is On");
        }
        else{
            api->getLogger()->logDebug(GwLog::DEBUG,"1Wire Mode is Off");
        }

        // Settings for NMEA0183
        String nmea0183Mode = api->getConfig()->getConfigItem(api->getConfig()->serialDirection,true)->asString();
        api->getLogger()->logDebug(GwLog::DEBUG,"NMEA0183 Mode is: %s", nmea0183Mode);
         pinMode(OBP_DIRECTION_PIN, OUTPUT);
        if(String(nmea0183Mode) == "receive" || String(nmea0183Mode) == "off"){
            digitalWrite(OBP_DIRECTION_PIN, false);
        }
        if(String(nmea0183Mode) == "send"){
            digitalWrite(OBP_DIRECTION_PIN, true);
        }
        
        // Settings for backlight
        String backlightMode = api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();
        api->getLogger()->logDebug(GwLog::DEBUG,"Backlight Mode is: %s", backlightMode);
        if(String(backlightMode) == "On"){
            setPortPin(OBP_BACKLIGHT_LED, true);
        }
        if(String(backlightMode) == "Off"){
            setPortPin(OBP_BACKLIGHT_LED, false);
        }
        if(String(backlightMode) == "Control by Key"){
            setPortPin(OBP_BACKLIGHT_LED, false);
        }

        // Settings flash LED mode
        String ledMode = api->getConfig()->getConfigItem(api->getConfig()->flashLED,true)->asString();
        api->getLogger()->logDebug(GwLog::DEBUG,"Backlight Mode is: %s", ledMode);
        if(String(ledMode) == "Off"){
            blinkingLED = false;
        }
        if(String(ledMode) == "Limits Overrun"){
            blinkingLED = true;
        }

        // Start serial stream and take over GPS data stream form internal GPS
        bool gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asBoolean();
        if(gpsOn == true){
            Serial2.begin(9600, SERIAL_8N1, OBP_GPS_TX, -1); // GPS RX unused in hardware (-1)
            if (!Serial2) {     
                api->getLogger()->logDebug(GwLog::ERROR,"GPS modul NEO-6M not found, check wiring");
                gps_ready = false;
                }
            else{
                api->getLogger()->logDebug(GwLog::DEBUG,"GPS modul NEO-M6 found");
                NMEA0183.SetMessageStream(&Serial2);
                NMEA0183.Open();
                gps_ready = true;
            }
        }

        // Marker for init complete
        // Used in OBP60Task()
        initComplete = true;

        // Buzzer tone for initialization finish
        buzPower = uint(api->getConfig()->getConfigItem(api->getConfig()->buzzerPower,true)->asInt());
        buzzer(TONE4, buzPower, 500);

    }

}

typedef struct {
        int page0=0;
        QueueHandle_t queue;
    } MyData;

void keyboardTask(void *param){
    MyData *data=(MyData *)param;
    int page=data->page0;
    while (true){
        //send a key event 
        xQueueSend(data->queue, &page, 0);
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
    extern PageDescription registerPageOneValue;
    //we add the variable to our list
    list.add(&registerPageOneValue);
    extern PageDescription registerPageTwoValues;
    list.add(&registerPageTwoValues);
    extern PageDescription registerPageThreeValuese;
    list.add(&registerPageThreeValuese);
    extern PageDescription registerPageForValues;
    list.add(&registerPageForValues);
    extern PageDescription registerPageApparentWind;
    list.add(&registerPageApparentWind);
}

void OBP60Task(GwApi *api){
    GwLog *logger=api->getLogger();
    GwConfigHandler *config=api->getConfig();
    PageList allPages;
    registerAllPages(allPages);
    LOG_DEBUG(GwLog::LOG,"obp60task started");
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
    MyData allParameters;
    allParameters.page0=3;
    allParameters.queue=xQueueCreate(10,sizeof(int));
    xTaskCreate(keyboardTask,"keyboard",2000,&allParameters,0,NULL);


    //loop
    LOG_DEBUG(GwLog::LOG,"obp60task: start mainloop");
    int pageNumber=0;
    int lastPage=pageNumber;
    while (true){
        delay(1000);
        //check if there is a keyboard message
        int keyboardMessage=-1;
        if (xQueueReceive(allParameters.queue,&keyboardMessage,0)){
            LOG_DEBUG(GwLog::LOG,"new page from keyboard %d",keyboardMessage);
            Page *currentPage=pages[pageNumber].page;
            if (currentPage ){
                keyboardMessage=currentPage->handleKey(keyboardMessage);
            }
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
            LOG_DEBUG(GwLog::DEBUG,"calling page %d",pageNumber);
            currentPage->display(commonData,pages[pageNumber].parameters);
        }

    }
    vTaskDelete(NULL);

}

#endif