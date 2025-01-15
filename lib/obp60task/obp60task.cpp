#ifdef BOARD_OBP60S3
#include "obp60task.h"
#include "Pagedata.h"                   // Data exchange for pages
#include "OBP60Hardware.h"              // PIN definitions
#include <Wire.h>                       // I2C connections
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

#include "LedSpiTask.h"

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
    hardwareInit(api);

    // Init power rail 5.0V
    String powermode = api->getConfig()->getConfigItem(api->getConfig()->powerMode,true)->asString();
    api->getLogger()->logDebug(GwLog::DEBUG,"Power Mode is: %s", powermode.c_str());
    if(powermode == "Max Power" || powermode == "Only 5.0V"){
        #ifdef HARDWARE_V21
        setPortPin(OBP_POWER_50, true); // Power on 5.0V rail
        #endif
        #ifdef HARDWARE_LIGHT
        setPortPin(OBP_POWER_EPD, true);// Power on ePaper display
        #endif
    }
    else{
        #ifdef HARDWARE_V21
        setPortPin(OBP_POWER_50, false); // Power off 5.0V rail
        #endif
        #ifdef HARDWARE_LIGHT
        setPortPin(OBP_POWER_EPD, false);// Power off ePaper display
        #endif
    }

    // Settings for e-paper display
    String fastrefresh = api->getConfig()->getConfigItem(api->getConfig()->fastRefresh,true)->asString();
    api->getLogger()->logDebug(GwLog::DEBUG,"Fast Refresh Mode is: %s", fastrefresh.c_str());
    #ifdef DISPLAY_GDEY042T81
    if(fastrefresh == "true"){
        static const bool useFastFullUpdate = true;   // Enable fast full display update only for GDEY042T81
    }
    #endif

    // Get CPU speed
    int freq = getCpuFrequencyMhz();
    api->getLogger()->logDebug(GwLog::LOG,"CPU speed at boot: %i MHz", freq);
    
    // Settings for backlight
    String backlightMode = api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();
    api->getLogger()->logDebug(GwLog::DEBUG,"Backlight Mode is: %s", backlightMode.c_str());
    uint brightness = uint(api->getConfig()->getConfigItem(api->getConfig()->blBrightness,true)->asInt());
    String backlightColor = api->getConfig()->getConfigItem(api->getConfig()->blColor,true)->asString();
    if(String(backlightMode) == "On"){
           setBacklightLED(brightness, colorMapping(backlightColor));
    }
    else if(String(backlightMode) == "Off"){
           setBacklightLED(0, COLOR_BLACK); // Backlight LEDs off (blue without britghness)
    }
    else if(String(backlightMode) == "Control by Key"){
           setBacklightLED(0, COLOR_BLUE); // Backlight LEDs off (blue without britghness)
    }

    // Settings flash LED mode
    String ledMode = api->getConfig()->getConfigItem(api->getConfig()->flashLED,true)->asString();
    api->getLogger()->logDebug(GwLog::DEBUG,"LED Mode is: %s", ledMode.c_str());
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
    extern PageDescription registerPageWind;
    list.add(&registerPageWind);
    extern PageDescription registerPageWindRose;
    list.add(&registerPageWindRose);
    extern PageDescription registerPageWindRoseFlex;
    list.add(&registerPageWindRoseFlex); // 
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
    extern PageDescription registerPageXTETrack;
    list.add(&registerPageXTETrack);
    extern PageDescription registerPageFluid;
    list.add(&registerPageFluid);
}

// Undervoltage detection for shutdown display
void underVoltageDetection(GwApi *api, CommonData &common){
    // Read settings
    float vslope = uint(api->getConfig()->getConfigItem(api->getConfig()->vSlope,true)->asFloat());
    float voffset = uint(api->getConfig()->getConfigItem(api->getConfig()->vOffset,true)->asFloat());
    // Read supply voltage
    float actVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // V = 1/20 * Vin
    actVoltage = actVoltage * vslope + voffset;
    if(actVoltage < MIN_VOLTAGE){
        // Switch off all power lines
        setPortPin(OBP_BACKLIGHT_LED, false);   // Backlight Off
        setFlashLED(false);                     // Flash LED Off            
        buzzer(TONE4, 20);                      // Buzzer tone 4kHz 20ms
        setPortPin(OBP_POWER_50, false);        // Power rail 5.0V Off
        // Shutdown EInk display
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update
        getdisplay().fillScreen(common.bgcolor);       // Clear screen
        getdisplay().setTextColor(common.fgcolor);
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
//    vTaskDelete(NULL);
//    return;
    GwLog *logger=api->getLogger();
    GwConfigHandler *config=api->getConfig();
    startLedTask(api);
    PageList allPages;
    registerAllPages(allPages);
    CommonData commonData;
    commonData.logger=logger;
    commonData.config=config;

    // Keyboard coordinates for page footer
    initKeys(commonData);

    tN2kMsg N2kMsg;

    LOG_DEBUG(GwLog::LOG,"obp60task started");
    for (auto it=allPages.pages.begin();it != allPages.pages.end();it++){
        LOG_DEBUG(GwLog::LOG,"found registered page %s",(*it)->pageName.c_str());
    }

    // Init E-Ink display
    String displaymode = api->getConfig()->getConfigItem(api->getConfig()->display,true)->asString();
    String displaycolor = api->getConfig()->getConfigItem(api->getConfig()->displaycolor,true)->asString();
    if (displaycolor == "Normal") {
        commonData.fgcolor = GxEPD_BLACK;
        commonData.bgcolor = GxEPD_WHITE;
    }
    else{
        commonData.fgcolor = GxEPD_WHITE;
        commonData.bgcolor = GxEPD_BLACK;
    }
    String systemname = api->getConfig()->getConfigItem(api->getConfig()->systemName,true)->asString();
    String wifipass = api->getConfig()->getConfigItem(api->getConfig()->apPassword,true)->asString();
    bool refreshmode = api->getConfig()->getConfigItem(api->getConfig()->refresh,true)->asBoolean();
    String fastrefresh = api->getConfig()->getConfigItem(api->getConfig()->fastRefresh,true)->asString();
    uint fullrefreshtime = uint(api->getConfig()->getConfigItem(api->getConfig()->fullRefreshTime,true)->asInt());

    #ifdef DISPLAY_GDEY042T81
        getdisplay().init(115200, true, 2, false);  // Use this for Waveshare boards with "clever" reset circuit, 2ms reset pulse
    #else
        getdisplay().init(115200);                  // Init for normal displays
    #endif

    getdisplay().setRotation(0);                 // Set display orientation (horizontal)
    getdisplay().setFullWindow();                // Set full Refresh
    getdisplay().firstPage();                    // set first page
    getdisplay().fillScreen(commonData.bgcolor);
    getdisplay().setTextColor(commonData.fgcolor);
    getdisplay().nextPage();                     // Full Refresh
    getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update
    getdisplay().fillScreen(commonData.bgcolor);
    getdisplay().nextPage();                     // Fast Refresh
    getdisplay().nextPage();                     // Fast Refresh
    if(String(displaymode) == "Logo + QR Code" || String(displaymode) == "Logo"){
        getdisplay().fillScreen(commonData.bgcolor);
        getdisplay().drawBitmap(0, 0, gImage_Logo_OBP_400x300_sw, getdisplay().width(), getdisplay().height(), commonData.fgcolor); // Draw start logo
        getdisplay().nextPage();                 // Fast Refresh
        getdisplay().nextPage();                 // Fast Refresh
        delay(SHOW_TIME);                        // Logo show time
        if(String(displaymode) == "Logo + QR Code"){
            getdisplay().fillScreen(commonData.bgcolor);
            qrWiFi(systemname, wifipass, commonData.fgcolor, commonData.bgcolor);  // Show QR code for WiFi connection
            getdisplay().nextPage();             // Fast Refresh
            getdisplay().nextPage();             // Fast Refresh
            delay(SHOW_TIME);                    // QR code show time
        }
        getdisplay().fillScreen(commonData.bgcolor);
        getdisplay().nextPage();                 // Fast Refresh
        getdisplay().nextPage();                 // Fast Refresh
    }
    
    // Init pages
    int numPages=1;
    PageStruct pages[MAX_PAGE_NUMBER];
    // Set start page
    int pageNumber = int(api->getConfig()->getConfigItem(api->getConfig()->startPage,true)->asInt()) - 1;
    int lastPage=pageNumber;

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
       pages[i].parameters.pageNumber = i + 1;
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

    // Display screenshot handler for HTTP request
    // http://192.168.15.1/api/user/OBP60Task/screenshot
    api->registerRequestHandler("screenshot", [api, &pageNumber, pages](AsyncWebServerRequest *request) {
        doImageRequest(api, &pageNumber, pages, request);
    });

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
    String gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asString();
    String tz = api->getConfig()->getConfigItem(api->getConfig()->timeZone,true)->asString();

    commonData.backlight.mode = backlightMapping(config->getConfigItem(config->backlight,true)->asString());
    commonData.backlight.color = colorMapping(config->getConfigItem(config->blColor,true)->asString());
    commonData.backlight.brightness = 2.55 * uint(config->getConfigItem(config->blBrightness,true)->asInt());

    bool uvoltage = api->getConfig()->getConfigItem(api->getConfig()->underVoltage,true)->asBoolean();
    String cpuspeed = api->getConfig()->getConfigItem(api->getConfig()->cpuSpeed,true)->asString();
    uint hdopAccuracy = uint(api->getConfig()->getConfigItem(api->getConfig()->hdopAccuracy,true)->asInt());

    // refreshmode defined in init section

    // Boat values for main loop
    GwApi::BoatValue *date = boatValues.findValueOrCreate("GPSD");      // Load GpsDate
    GwApi::BoatValue *time = boatValues.findValueOrCreate("GPST");      // Load GpsTime
    GwApi::BoatValue *lat = boatValues.findValueOrCreate("LAT");        // Load GpsLatitude
    GwApi::BoatValue *lon = boatValues.findValueOrCreate("LON");        // Load GpsLongitude
    GwApi::BoatValue *hdop = boatValues.findValueOrCreate("HDOP");       // Load GpsHDOP

    LOG_DEBUG(GwLog::LOG,"obp60task: start mainloop");
    
    commonData.time = boatValues.findValueOrCreate("GPST");      // Load GpsTime
    commonData.date = boatValues.findValueOrCreate("GPSD");      // Load GpsTime
    bool delayedDisplayUpdate = false;  // If select a new pages then make a delayed full display update
    bool cpuspeedsetted = false;    // Marker for change CPU speed 
    long firststart = millis();     // First start
    long starttime0 = millis();     // Mainloop
    long starttime1 = millis();     // Full display refresh for the first 5 min (more often as normal)
    long starttime2 = millis();     // Full display refresh after 5 min
    long starttime3 = millis();     // Display update all 1s
    long starttime4 = millis();     // Delayed display update after 4s when select a new page
    long starttime5 = millis();     // Calculate sunrise and sunset all 1s

    pages[pageNumber].page->setupKeys(); // Initialize keys for first page

    // Main loop runs with 100ms
    //####################################################################################
    
    while (true){
        delay(100);     // Delay 100ms (loop time)

    
        // Undervoltage detection
        if(uvoltage == true){
            underVoltageDetection(api, commonData);
        }  

        // Set CPU speed after boot after 1min 
        if(millis() > firststart + (1 * 60 * 1000) && cpuspeedsetted == false){
            if(String(cpuspeed) == "80"){
                setCpuFrequencyMhz(80);
            }
            if(String(cpuspeed) == "160"){
                setCpuFrequencyMhz(160);
            }
            if(String(cpuspeed) == "240"){
                setCpuFrequencyMhz(240);
            }
            int freq = getCpuFrequencyMhz();
            api->getLogger()->logDebug(GwLog::LOG,"CPU speed: %i MHz", freq);
            cpuspeedsetted = true;
        }

        if(millis() > starttime0 + 100){
            starttime0 = millis();
            commonData.data=shared->getSensorData();
            commonData.data.actpage = pageNumber + 1;
            commonData.data.maxpage = numPages;
            
            // If GPS fix then LED off (HDOP)
            if(String(gpsFix) == "GPS Fix Lost" && hdop->value <= hdopAccuracy && hdop->valid == true){
                setFlashLED(false);
            }
            // If missing GPS fix then LED on
            if((String(gpsFix) == "GPS Fix Lost" && hdop->value > hdopAccuracy && hdop->valid == true) || (String(gpsFix) == "GPS Fix Lost" && hdop->valid == false)){
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
                    if (commonData.backlight.mode == BacklightMode::KEY) {
                    // if(String(backlight) == "Control by Key"){
                        if(keyboardMessage == 6){
                            LOG_DEBUG(GwLog::LOG,"Toggle Backlight LED");
                            toggleBacklightLED(commonData.backlight.brightness, commonData.backlight.color);
                        }
                    }
                    // #9 Swipe right or #4 key right
                    if ((keyboardMessage == 9) or (keyboardMessage == 4))
                    {
                        pageNumber++;
                        if (pageNumber >= numPages){
                            pageNumber = 0;
                        }
                        commonData.data.actpage = pageNumber + 1;
                        commonData.data.maxpage = numPages;
                    }
                    // #10 Swipe left or #3 key left
                    if ((keyboardMessage == 10) or (keyboardMessage == 3))
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
                    if (commonData.backlight.mode == BacklightMode::SUN) {
                    // if(String(backlight) == "Control by Sun"){
                        if(commonData.sundata.sunDown == true){
                            setBacklightLED(commonData.backlight.brightness, commonData.backlight.color);
                        }
                        else{
                            setBacklightLED(0, COLOR_BLUE); // Backlight LEDs off (blue without britghness)
                        }           

                    }
                }
            }
            
            // Full display update afer a new selected page and 4s wait time
            if(millis() > starttime4 + 4000 && delayedDisplayUpdate == true){
                starttime1 = millis();
                starttime2 = millis();
                getdisplay().setFullWindow();    // Set full update
                getdisplay().nextPage();
                if(fastrefresh == "false"){
                    getdisplay().fillScreen(commonData.fgcolor); // Clear display
                    getdisplay().nextPage();                     // Full update
                    getdisplay().fillScreen(commonData.bgcolor); // Clear display
                    getdisplay().nextPage();                     // Full update
                }
                delayedDisplayUpdate = false;
            }

            // Subtask E-Ink full refresh all 1 min for the first 5 min after power on or restart
            // This needs for a better display contrast after power on in cold or warm environments
            if(millis() < firststart + (5 * 60 * 1000) && millis() > starttime1 + (60 * 1000)){
                starttime1 = millis();
                starttime2 = millis();
                LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh first 5 min");
                getdisplay().setFullWindow();    // Set full update
                getdisplay().nextPage();
                if(fastrefresh == "false"){
                    getdisplay().fillScreen(commonData.fgcolor); // Clear display
                    getdisplay().nextPage();                     // Full update
                    getdisplay().fillScreen(commonData.bgcolor); // Clear display
                    getdisplay().nextPage();                     // Full update
                }
            }

            // Subtask E-Ink full refresh
            if(millis() > starttime2 + fullrefreshtime * 60 * 1000){
                starttime2 = millis();
                LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh");
                getdisplay().setFullWindow();    // Set full update
                getdisplay().nextPage();
                if(fastrefresh == "false"){
                    getdisplay().fillScreen(commonData.fgcolor); // Clear display
                    getdisplay().nextPage();                     // Full update
                    getdisplay().fillScreen(commonData.bgcolor); // Clear display
                    getdisplay().nextPage();                     // Full update
                }
            }
            
            // Refresh display data all 1s
            if(millis() > starttime3 + 1000){
                starttime3 = millis();
                //refresh data from api
                api->getBoatDataValues(boatValues.numValues,boatValues.allBoatValues);
                api->getStatus(commonData.status);

                // Show header if enabled
                getdisplay().fillRect(0, 0, getdisplay().width(), getdisplay().height(), commonData.bgcolor);   // Clear display
                if (pages[pageNumber].description && pages[pageNumber].description->header){
                    // build header using commonData
                    getdisplay().fillScreen(commonData.bgcolor);  // Clear display
                    displayHeader(commonData, date, time, hdop);  // Show page header
                }

                // Call the particular page
                Page *currentPage=pages[pageNumber].page;
                if (currentPage == NULL){
                    LOG_DEBUG(GwLog::ERROR,"page number %d not found",pageNumber);
                    // Error handling for missing page
                }
                else{
                    if (lastPage != pageNumber){
                        if (hasFRAM) fram.write(FRAM_PAGE_NO, pageNumber); // remember page for device restart
                        currentPage->setupKeys();
                        currentPage->displayNew(pages[pageNumber].parameters);
                        lastPage=pageNumber;
                    }
                    //call the page code
                    LOG_DEBUG(GwLog::DEBUG,"calling page %d",pageNumber);
                    // Show footer if enabled (together with header)
                    if (pages[pageNumber].description && pages[pageNumber].description->header){
                        displayFooter(commonData);
                    }
                    currentPage->displayPage(pages[pageNumber].parameters);
                }
            }
        }
    }
    vTaskDelete(NULL);

}

#endif
