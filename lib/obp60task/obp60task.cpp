#ifdef BOARD_OBP60S3
#include "obp60task.h"
#include "Pagedata.h"
#include "OBP60Hardware.h"              // PIN definitions
#include <Wire.h>                       // I2C connections
#include <RTClib.h>                     // DS1388 RTC
#include <MCP23017.h>                   // MCP23017 extension Port
#include <N2kTypes.h>                   // NMEA2000
#include <N2kMessages.h>
#include <NMEA0183.h>                   // NMEA0183
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <GxEPD2_BW.h>                  // GxEPD2 lib for black 6 white E-Ink displays
#include "OBP60Extensions.h"            // Functions lib for extension board
#include "OBP60Keypad.h"                // Functions for keypad

// True type character sets includes
// See OBP60ExtensionPort.cpp

// Pictures
//#include GxEPD_BitmapExamples         // Example picture
#include "MFD_OBP60_400x300_sw.h"       // MFD with logo
#include "Logo_OBP_400x300_sw.h"        // OBP Logo
#include "OBP60QRWiFi.h"                // Functions lib for WiFi QR code
#include "OBPSensorTask.h"              // Functions lib for sensor data

// RTC DS1388
RTC_DS1388 ds1388;

// Global vars
bool initComplete = false;      // Initialization complete
int taskRunCounter = 0;         // Task couter for loop section

// Hardware initialization before start all services
//####################################################################################
void OBP60Init(GwApi *api){

    // Set a new device name and hidden the original name in the main config
    String devicename = api->getConfig()->getConfigItem(api->getConfig()->deviceName,true)->asString();
    api->getConfig()->setValue(GwConfigDefinitions::systemName, devicename, GwConfigInterface::ConfigType::HIDDEN);

    api->getLogger()->logDebug(GwLog::LOG,"obp60init running");
    
    // Check I2C devices
    

    // Init hardware
    hardwareInit();
    // static const bool useFastFullUpdate = true; // For high speed full update e-paper
    static const bool useFastFullUpdate = false; // For normal speed full update e-paper
    /*
    setCpuFrequencyMhz(80);
    int freq = getCpuFrequencyMhz();
    api->getLogger()->logDebug(GwLog::LOG,"CPU speed: %i", freq);
    */
    
    // Settings for backlight
    String backlightMode = api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();
    api->getLogger()->logDebug(GwLog::DEBUG,"Backlight Mode is: %s", backlightMode);
    uint brightness = uint(api->getConfig()->getConfigItem(api->getConfig()->blBrightness,true)->asInt());
    String backlightColor = api->getConfig()->getConfigItem(api->getConfig()->blColor,true)->asString();
    if(String(backlightMode) == "On"){
           setBacklightLED(brightness, colorMapping(backlightColor));
    }
    if(String(backlightMode) == "Off"){
           setBacklightLED(0, CHSV(HUE_BLUE, 255, 0)); // Backlight LEDs off (blue without britghness)
    }
    if(String(backlightMode) == "Control by Key"){
           setBacklightLED(0, CHSV(HUE_BLUE, 255, 0)); // Backlight LEDs off (blue without britghness)
    }

    // Settings flash LED mode
    String ledMode = api->getConfig()->getConfigItem(api->getConfig()->flashLED,true)->asString();
    api->getLogger()->logDebug(GwLog::DEBUG,"Backlight Mode is: %s", ledMode);
    if(String(ledMode) == "Off"){
        setBlinkingLED(false);
    }

    // Marker for init complete
    // Used in OBP60Task()
    initComplete = true;

    // Buzzer tone for initialization finish
    setBuzzerPower(uint(api->getConfig()->getConfigItem(api->getConfig()->buzzerPower,true)->asInt()));
    buzzer(TONE4, 500);

}

typedef struct {
        int page0=0;
        QueueHandle_t queue;
        GwLog* logger = NULL;
//        GwApi* api = NULL;
        uint sensitivity = 100;
    } MyData;

// Keyboard Task
void keyboardTask(void *param){
    MyData *data=(MyData *)param;

    int keycode = 0;
    data->logger->logDebug(GwLog::LOG,"Start keyboard task");

    // Loop for keyboard task
    while (true){
        keycode = readKeypad(data->sensitivity);
        //send a key event
        if(keycode != 0){
            xQueueSend(data->queue, &keycode, 0);
            data->logger->logDebug(GwLog::LOG,"Send keycode: %d", keycode);
        }
        delay(20);      // 50Hz update rate (20ms)
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
    extern PageDescription registerPageThreeValues;
    list.add(&registerPageThreeValues);
    extern PageDescription registerPageFourValues;
    list.add(&registerPageFourValues);
    extern PageDescription registerPageFourValues2;
    list.add(&registerPageFourValues2);
    extern PageDescription registerPageApparentWind;
    list.add(&registerPageApparentWind);
    extern PageDescription registerPageWindRose;
    list.add(&registerPageWindRose);
    extern PageDescription registerPageVoltage;
    list.add(&registerPageVoltage);
    extern PageDescription registerPageDST810;
    list.add(&registerPageDST810);
    extern PageDescription registerPageClock;
    list.add(&registerPageClock);
    extern PageDescription registerPageWhite;
    list.add(&registerPageWhite);
    extern PageDescription registerPageBME280;
    list.add(&registerPageBME280);
    extern PageDescription registerPageRudderPosition;
    list.add(&registerPageRudderPosition);
    extern PageDescription registerPageKeelPosition;
    list.add(&registerPageKeelPosition);
    extern PageDescription registerPageBattery;
    list.add(&registerPageBattery);
    extern PageDescription registerPageBattery2;
    list.add(&registerPageBattery2);
    extern PageDescription registerPageRollPitch;
    list.add(&registerPageRollPitch);
    extern PageDescription registerPageSolar;
    list.add(&registerPageSolar);
    extern PageDescription registerPageGenerator;
    list.add(&registerPageGenerator);
}

// Undervoltage detection for shutdown display
void underVoltageDetection(GwApi *api){
    int textcolor = GxEPD_BLACK;
    int pixelcolor = GxEPD_BLACK;
    int bgcolor = GxEPD_WHITE;
    // Read settings
    String displaycolor = api->getConfig()->getConfigItem(api->getConfig()->displaycolor,true)->asString();
    float vslope = uint(api->getConfig()->getConfigItem(api->getConfig()->vSlope,true)->asFloat());
    float voffset = uint(api->getConfig()->getConfigItem(api->getConfig()->vOffset,true)->asFloat());
    // Read supplay voltage
    float actVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // V = 1/20 * Vin
    actVoltage = actVoltage * vslope + voffset;
    if(actVoltage < MIN_VOLTAGE){
        // Switch off all power lines
        setPortPin(OBP_BACKLIGHT_LED, false);   // Backlight Off
        setFlashLED(false);                     // Flash LED Off            
        buzzer(TONE4, 20);                      // Buzzer tone 4kHz 20ms
        setPortPin(OBP_POWER_50, false);        // Power rail 5.0V Off
        // Shutdown EInk display
        if(displaycolor == "Normal"){
            textcolor = GxEPD_BLACK;
            bgcolor = GxEPD_WHITE;
        }
        else{
            textcolor = GxEPD_WHITE;
            bgcolor = GxEPD_BLACK;
        }
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update
        getdisplay().fillScreen(bgcolor);       // Clear screen
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(65, 150);
        getdisplay().print("Undervoltage");
        getdisplay().nextPage();                // Partial update
        getdisplay().powerOff();                // Display power off
        // Stop system
        while(true){
            esp_deep_sleep_start();             // Deep Sleep without weakup. Weakup only after power cycle (restart).
        }
    }
}

// OBP60 Task
//####################################################################################
void OBP60Task(GwApi *api){
    GwLog *logger=api->getLogger();
    GwConfigHandler *config=api->getConfig();
    PageList allPages;
    registerAllPages(allPages);

    tN2kMsg N2kMsg;

    LOG_DEBUG(GwLog::LOG,"obp60task started");
    for (auto it=allPages.pages.begin();it != allPages.pages.end();it++){
        LOG_DEBUG(GwLog::LOG,"found registered page %s",(*it)->pageName.c_str());
    }
    
    // Init E-Ink display
    String displaymode = api->getConfig()->getConfigItem(api->getConfig()->display,true)->asString();
    String displaycolor = api->getConfig()->getConfigItem(api->getConfig()->displaycolor,true)->asString();
    String systemname = api->getConfig()->getConfigItem(api->getConfig()->systemName,true)->asString();
    String wifipass = api->getConfig()->getConfigItem(api->getConfig()->apPassword,true)->asString();
    bool refreshmode = api->getConfig()->getConfigItem(api->getConfig()->refresh,true)->asBoolean();
    int textcolor = GxEPD_BLACK;
    int pixelcolor = GxEPD_BLACK;
    int bgcolor = GxEPD_WHITE;

    getdisplay().init(115200);   // Init for nolrmal displays
 //   getdisplay().init(115200, true, 2, false);   // Use this for Waveshare boards with "clever" reset circuit, 2ms reset pulse
    getdisplay().setRotation(0);                 // Set display orientation (horizontal)
    if(displaycolor == "Normal"){
        textcolor = GxEPD_BLACK;
        pixelcolor = GxEPD_BLACK;
        bgcolor = GxEPD_WHITE;
    }
    else{
        textcolor = GxEPD_WHITE;
        pixelcolor = GxEPD_WHITE;
        bgcolor = GxEPD_BLACK;
    }
    getdisplay().setFullWindow();                // Set full Refresh
    getdisplay().firstPage();                    // set first page
    getdisplay().fillScreen(bgcolor);            // Draw white sreen
    getdisplay().setTextColor(textcolor);        // Set display color
    getdisplay().nextPage();                     // Full Refresh
    getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update
    getdisplay().fillScreen(bgcolor);            // Draw white sreen
    getdisplay().nextPage();                     // Fast Refresh
    getdisplay().nextPage();                     // Fast Refresh
    if(String(displaymode) == "Logo + QR Code" || String(displaymode) == "Logo"){
        getdisplay().fillScreen(bgcolor);        // Draw white sreen
        getdisplay().drawBitmap(0, 0, gImage_Logo_OBP_400x300_sw, getdisplay().width(), getdisplay().height(), pixelcolor); // Draw start logo
        getdisplay().nextPage();                 // Fast Refresh
        getdisplay().nextPage();                 // Fast Refresh
        delay(SHOW_TIME);                        // Logo show time
        if(String(displaymode) == "Logo + QR Code"){
            getdisplay().fillScreen(bgcolor);    // Draw white sreen
            qrWiFi(systemname, wifipass, displaycolor);  // Show QR code for WiFi connection
            getdisplay().nextPage();             // Fast Refresh
            getdisplay().nextPage();             // Fast Refresh
            delay(SHOW_TIME);                    // QR code show time
        }
        getdisplay().fillScreen(bgcolor);        // Draw white sreen
        getdisplay().nextPage();                 // Fast Refresh
        getdisplay().nextPage();                 // Fast Refresh
    }
    
    // Init pages
    int numPages=1;
    PageStruct pages[MAX_PAGE_NUMBER];
    CommonData commonData;
    commonData.logger=logger;
    commonData.config=config;
    BoatValueList boatValues; //all the boat values for the api query
    //commonData.distanceformat=config->getString(xxx);
    //add all necessary data to common data

    //fill the page data from config
    numPages=config->getInt(config->visiblePages,1);
    if (numPages < 1) numPages=1;
    if (numPages >= MAX_PAGE_NUMBER) numPages=MAX_PAGE_NUMBER;
    LOG_DEBUG(GwLog::LOG,"Number of pages %d",numPages);
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
    allParameters.logger=api->getLogger();
    allParameters.page0=3;
    allParameters.queue=xQueueCreate(10,sizeof(int));
    allParameters.sensitivity= api->getConfig()->getInt(GwConfigDefinitions::tSensitivity);
    xTaskCreate(keyboardTask,"keyboard",2000,&allParameters,configMAX_PRIORITIES-1,NULL);
    SharedData *shared=new SharedData(api);
    createSensorTask(shared);

    // Task Loop
    //####################################################################################

    // Configuration values for main loop
    String gpsFix = api->getConfig()->getConfigItem(api->getConfig()->flashLED,true)->asString();
    String backlight = api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();
    String gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asString();
    String tz = api->getConfig()->getConfigItem(api->getConfig()->timeZone,true)->asString();
    String backlightColor = api->getConfig()->getConfigItem(api->getConfig()->blColor,true)->asString();
    CHSV color = colorMapping(backlightColor);
    uint brightness = 2.55 * uint(api->getConfig()->getConfigItem(api->getConfig()->blBrightness,true)->asInt());
    bool uvoltage = api->getConfig()->getConfigItem(api->getConfig()->underVoltage,true)->asBoolean();

    // refreshmode defined in init section
    // displaycolor defined in init section
    // textcolor defined in init section
    // pixelcolor defined in init section
    // bgcolor defined in init section

    // Boat values for main loop
    GwApi::BoatValue *date = boatValues.findValueOrCreate("GPSD");      // Load GpsDate
    GwApi::BoatValue *time = boatValues.findValueOrCreate("GPST");      // Load GpsTime
    GwApi::BoatValue *lat = boatValues.findValueOrCreate("LAT");        // Load GpsLatitude
    GwApi::BoatValue *lon = boatValues.findValueOrCreate("LON");        // Load GpsLongitude
    GwApi::BoatValue *hdop = boatValues.findValueOrCreate("HDOP");       // Load GpsHDOP

    LOG_DEBUG(GwLog::LOG,"obp60task: start mainloop");
    // Set start page
    int pageNumber = int(api->getConfig()->getConfigItem(api->getConfig()->startPage,true)->asInt()) - 1;
    int lastPage=pageNumber;
    
    commonData.time = boatValues.findValueOrCreate("GPST");      // Load GpsTime
    commonData.date = boatValues.findValueOrCreate("GPSD");      // Load GpsTime
    bool delayedDisplayUpdate = false;  // If select a new pages then make a delayed full display update
    long firststart = millis();     // First start
    long starttime0 = millis();     // Mainloop
    long starttime1 = millis();     // Full display refresh for the first 5 min (more often as normal)
    long starttime2 = millis();     // Full display refresh after 5 min
    long starttime3 = millis();     // Display update all 1s
    long starttime4 = millis();     // Delayed display update after 4s when select a new page
    long starttime5 = millis();     // Calculate sunrise and sunset all 1s
    
    // Main loop runs with 100ms
    //####################################################################################
    
    while (true){
        delay(100);     // Delay 100ms (loop time)

    
        // Undervoltage detection
        if(uvoltage == true){
            underVoltageDetection(api);
        }  

        if(millis() > starttime0 + 100){
            starttime0 = millis();
            commonData.data=shared->getSensorData();
            commonData.data.actpage = pageNumber + 1;
            commonData.data.maxpage = numPages;
            
            // If GPS fix then LED off (HDOP)
            if(String(gpsFix) == "GPS Fix Lost" && date->valid == true){
                setFlashLED(false);
            }
            // If missing GPS fix then LED on
            if(String(gpsFix) == "GPS Fix Lost" && date->valid == false){
                setFlashLED(true);
            }
         
            // Check the keyboard message
            int keyboardMessage=0;
            while (xQueueReceive(allParameters.queue,&keyboardMessage,0)){
                LOG_DEBUG(GwLog::LOG,"new key from keyboard %d",keyboardMessage);
                
                Page *currentPage=pages[pageNumber].page;
                if (currentPage ){
                    keyboardMessage=currentPage->handleKey(keyboardMessage);
                }
                if (keyboardMessage > 0)
                {
                    // Decoding all key codes
                    // #6 Backlight on if key controled
                    if(String(backlight) == "Control by Key"){
                        if(keyboardMessage == 6){
                            LOG_DEBUG(GwLog::LOG,"Toggle Backlight LED");
                            toggleBacklightLED(brightness, color);
                        }
                    }
                    // #9 Swipe right
                    if (keyboardMessage == 9)
                    {
                        pageNumber++;
                        if (pageNumber >= numPages){
                            pageNumber = 0;
                        }
                        commonData.data.actpage = pageNumber + 1;
                        commonData.data.maxpage = numPages;
                    }
                    // #10 Swipe left
                    if (keyboardMessage == 10)
                    {
                        pageNumber--;
                        if (pageNumber < 0){
                            pageNumber = numPages - 1;
                        }
                        commonData.data.actpage = pageNumber + 1;
                        commonData.data.maxpage = numPages;
                    }
                  
                    // #9 or #10 Refresh display after a new page after 4s waiting time and if refresh is disabled
                    if(refreshmode == true && (keyboardMessage == 9 || keyboardMessage == 10)){
                        starttime4 = millis();
                        starttime2 = millis();      // Reset the timer for full display update
                        delayedDisplayUpdate = true;
                    }                 
                }
                LOG_DEBUG(GwLog::LOG,"set pagenumber to %d",pageNumber);
            }

            // Calculate sunrise, sunset and backlight control with sun status all 1s
            if(millis() > starttime5 + 1000){
                starttime5 = millis();
                if(time->valid == true && date->valid == true && lat->valid == true && lon->valid == true){
                    // Provide sundata to all pages
                    commonData.sundata = calcSunsetSunrise(api, time->value , date->value, lat->value, lon->value, tz.toDouble());
                    // Backlight with sun control
                    if(String(backlight) == "Control by Sun"){
                        if(commonData.sundata.sunDown == true){
                            setBacklightLED(brightness, color);
                        }
                        else{
                            setBacklightLED(0, CHSV(HUE_BLUE, 255, 0)); // Backlight LEDs off (blue without britghness)
                        }           

                    }
                }
            }
            
            // Full display update afer a new selected page and 4s wait time
            if(millis() > starttime4 + 4000 && delayedDisplayUpdate == true){
                getdisplay().setFullWindow();    // Set full update
                getdisplay().fillScreen(pixelcolor);// Clear display
                getdisplay().nextPage();         // Full update
                getdisplay().fillScreen(bgcolor);// Clear display
                getdisplay().nextPage();         // Full update
                delayedDisplayUpdate = false;
            }

            // Subtask E-Ink full refresh all 1 min for the first 5 min after power on or restart
            // This needs for a better display contrast after power on in cold or warm environments
            if(millis() < firststart + (5 * 60 * 1000) && millis() > starttime1 + (60 * 1000)){
                starttime1 = millis();
                LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh first 5 min");
                getdisplay().setFullWindow();    // Set full update
                getdisplay().fillScreen(pixelcolor);// Clear display
                getdisplay().nextPage();         // Full update
                getdisplay().fillScreen(bgcolor);// Clear display
                getdisplay().nextPage();         // Full update
            }

            // Subtask E-Ink full refresh
            //if(millis() > starttime2 + FULL_REFRESH_TIME * 1000){
            if(millis() > starttime2 + 1 * 60 * 1000){
                starttime2 = millis();
                LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh");
                getdisplay().setFullWindow();    // Set full update
                getdisplay().setFullWindow();    // Set full update
                getdisplay().fillScreen(pixelcolor);// Clear display
                getdisplay().nextPage();         // Full update
                getdisplay().fillScreen(bgcolor);// Clear display
                getdisplay().nextPage();         // Full update
            }
            
            // Refresh display data all 1s
            if(millis() > starttime3 + 1000){
                starttime3 = millis();
                //refresh data from api
                api->getBoatDataValues(boatValues.numValues,boatValues.allBoatValues);
                api->getStatus(commonData.status);

                // Show header if enabled
                getdisplay().fillRect(0, 0, getdisplay().width(), getdisplay().height(), bgcolor);   // Clear display
                if (pages[pageNumber].description && pages[pageNumber].description->header){
                    //build some header and footer using commonData
                    getdisplay().fillScreen(bgcolor);             // Clear display
                    displayHeader(commonData, date, time, hdop);  // Sown header
                }
                
                // Call the particular page
                Page *currentPage=pages[pageNumber].page;
                if (currentPage == NULL){
                    LOG_DEBUG(GwLog::ERROR,"page number %d not found",pageNumber);
                    // Error handling for missing page
                }
                else{
                    if (lastPage != pageNumber){
                        currentPage->displayNew(commonData,pages[pageNumber].parameters);
                        lastPage=pageNumber;
                    }
                    //call the page code
                    LOG_DEBUG(GwLog::DEBUG,"calling page %d",pageNumber);
                    currentPage->displayPage(commonData,pages[pageNumber].parameters);
                }
            }
        }
    }
    vTaskDelete(NULL);

}

#endif