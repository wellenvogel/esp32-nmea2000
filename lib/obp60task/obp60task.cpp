#ifdef BOARD_NODEMCU32S_OBP60
#include "obp60task.h"
#include "Pagedata.h"
#include "OBP60Hardware.h"              // PIN definitions

#include <Ticker.h>                     // Timer Lib for timer interrupts
#include <Wire.h>                       // I2C connections
#include <MCP23017.h>                   // MCP23017 extension Port
#include <Adafruit_Sensor.h>            // Adafruit Lib for sensors
#include <Adafruit_BME280.h>            // Adafruit Lib for BME280
#include <Adafruit_BMP280.h>            // Adafruit Lib for BMP280
#include <HTU21D.h>                     // Lib for SHT21/HTU21
#include <N2kTypes.h>                   // NMEA2000
#include <N2kMessages.h>
#include <NMEA0183.h>                   // NMEA0183
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <GxEPD.h>                      // GxEPD lib for E-Ink displays
#include "OBP60ExtensionPort.h"         // Functions lib for extension board
#include "OBP60Keypad.h"                // Functions for keypad

// True type character sets includes
// See OBP60ExtensionPort.cpp

// Pictures
//#include GxEPD_BitmapExamples         // Example picture
#include "MFD_OBP60_400x300_sw.h"       // MFD with logo
#include "Logo_OBP_400x300_sw.h"        // OBP Logo
#include "OBP60QRWiFi.h"                // Functions lib for WiFi QR code

#include "ObpNmea0183.h"
#include "GwNmea0183Msg.h"

tNMEA0183Msg NMEA0183Msg;
ObpNmea0183 NMEA0183;                   // Fixed Lib for NMEA0183
// tNMEA0183 NMEA0183;                  // Old lib with problems for NMEA0183

Adafruit_BME280 bme280;                 // Evironment sensor BME280
Adafruit_BMP280 bmp280;                 // Evironment sensor BMEP280
HTU21D  sht21(HTU21D_RES_RH12_TEMP14);  // Environment sensor SHT21 identical to HTU21

// Global vars
bool initComplete = false;      // Initialization complete
int taskRunCounter = 0;         // Task couter for loop section
bool gps_ready = false;         // GPS initialized and ready to use
bool BME280_ready = false;      // BME280 initialized and ready to use
bool BMP280_ready = false;      // BMP20 initialized and ready to use
bool SHT21_ready = false;       // SHT21 initialized and ready to use
double airhumidity = 0;         // Air Humitity value from environment sensor
double airtemperature = 0;      // Air Temperature value from environment sensor
double airpressure = 0;         // Ais pressure value from environment sensor

// Timer Interrupts for hardware functions
void underVoltageDetection();
Ticker Timer1(underVoltageDetection, 1);     // Start Timer1 with maximum speed with 1ms
Ticker Timer2(blinkingFlashLED, 500);

// Undervoltage function for shutdown display
void underVoltageDetection(){
    float actVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // V = 1/20 * Vin
    long starttime = 0;
    static bool undervoltage = false;

    if(actVoltage < MIN_VOLTAGE){
        if(undervoltage == false){
            starttime = millis();
            undervoltage = true;
        }
        if(millis() > starttime + POWER_FAIL_TIME){
            Timer1.stop();                          // Stop Timer1
            setPortPin(OBP_BACKLIGHT_LED, false);   // Backlight Off
            setPortPin(OBP_FLASH_LED, false);       // Flash LED Off
            setPortPin(OBP_POWER_33, false);        // Power rail 3.3V Off
            buzzer(TONE4, 20);                      // Buzzer tone 4kHz 20ms
            setPortPin(OBP_POWER_50, false);        // Power rail 5.0V Off
            // Shutdown EInk display
            display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
            display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false); // Partial update
            display.update();
            // Stop system
            while(true){
                esp_deep_sleep_start();             // Deep Sleep without weakup. Weakup only after power cycle (restart).
            }
        }
    }
    else{
        undervoltage = false;
    }
}

// Hardware initialization before start all services
//##################################################
void OBP60Init(GwApi *api){
    GwLog *logger = api->getLogger();
    api->getLogger()->logDebug(GwLog::LOG,"obp60init running");

    // Start timer interrupts
    bool uvoltage = api->getConfig()->getConfigItem(api->getConfig()->underVoltage,true)->asBoolean();
    if(uvoltage == true){
        Timer1.start();     // Start Timer1 for undervoltage detection
    }
    Timer2.start();         // Start Timer2 for blinking LED

    // Extension port MCP23017
    // Check I2C devices MCP23017
    Wire.begin(OBP_I2C_SDA, OBP_I2C_SCL);
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        api->getLogger()->logDebug(GwLog::ERROR,"MCP23017 not found, check wiring");
        initComplete = false;
    }
    else{ 
        // Init extension port
        MCP23017Init();

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
            setBlinkingLED(false);
        }

        // Start serial stream and take over GPS data stream form internal GPS
        String gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asString();
        if(String(gpsOn) == "NEO-6M"){
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
        if(String(gpsOn) == "NEO-M8N"){
            Serial2.begin(9600, SERIAL_8N1, OBP_GPS_TX, -1); // GPS RX unused in hardware (-1)
            if (!Serial2) {     
                api->getLogger()->logDebug(GwLog::ERROR,"GPS modul NEO-M8N not found, check wiring");
                gps_ready = false;
                }
            else{
                api->getLogger()->logDebug(GwLog::DEBUG,"GPS modul NEO-M8N found");
                NMEA0183.SetMessageStream(&Serial2);
                NMEA0183.Open();
                gps_ready = true;
            }
        }

        // Marker for init complete
        // Used in OBP60Task()
        initComplete = true;

        // Buzzer tone for initialization finish
        setBuzzerPower(uint(api->getConfig()->getConfigItem(api->getConfig()->buzzerPower,true)->asInt()));
        buzzer(TONE4, 500);

    }

    // Settings for temperature sensors
    String tempSensor = api->getConfig()->getConfigItem(api->getConfig()->useTempSensor,true)->asString();
    if(String(tempSensor) == "DS18B20"){
        api->getLogger()->logDebug(GwLog::DEBUG,"1Wire Mode is On");
    }
    else{
        api->getLogger()->logDebug(GwLog::DEBUG,"1Wire Mode is Off");
    }
    
    // Settings for environment sensors on I2C bus
    String envSensors=api->getConfig()->getConfigItem(api->getConfig()->useEnvSensor,true)->asString();

    if(String(envSensors) == "BME280"){
        if (!bme280.begin(BME280_I2C_ADDR)) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul BME280 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::DEBUG,"Modul BME280 found");
            airtemperature = bme280.readTemperature();
            airpressure = bme280.readPressure()/100;
            airhumidity = bme280.readHumidity();
            BME280_ready = true;
        }
    }
    else if(String(envSensors) == "BMP280"){
        if (!bmp280.begin(BMP280_I2C_ADDR)) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul BMP280 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::DEBUG,"Modul BMP280 found");
            airtemperature = bmp280.readTemperature();
            airpressure  =bmp280.readPressure()/100;
            BMP280_ready = true;
        }
    }
    else if(String(envSensors) == "SHT21"){
        if (!sht21.begin()) {
            api->getLogger()->logDebug(GwLog::ERROR,"Modul SHT21 not found, check wiring");
        }
        else{
            api->getLogger()->logDebug(GwLog::DEBUG,"Modul SHT21 found");
            airhumidity = sht21.readCompensatedHumidity();
            airtemperature = sht21.readTemperature();
            SHT21_ready = true;
        }
    }
    

}

typedef struct {
        int page0=0;
        QueueHandle_t queue;
        GwLog* logger = NULL;
    } MyData;

// Keyboard Task
//#######################################
int readKeypad();

void keyboardTask(void *param){
    MyData *data=(MyData *)param;
    int keycode = 0;
    data->logger->logDebug(GwLog::LOG,"Start keyboard task");

    // Loop for keyboard task
    while (true){
        keycode = readKeypad();
        //send a key event
        if(keycode != 0){
            xQueueSend(data->queue, &keycode, 0);
            data->logger->logDebug(GwLog::LOG,"Send keycode: %d", keycode);
        }

        delay(20);      // TP229-BSF working in 8 key mode with 64Hz update rate (15.6ms)
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
    extern PageDescription registerPageForValues;
    list.add(&registerPageForValues);
    extern PageDescription registerPageForValues2;
    list.add(&registerPageForValues2);
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
}

// OBP60 Task
//#######################################
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

    display.init();                         // Initialize and clear display
    display.setRotation(0);                 // Set display orientation (horizontal)
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
    display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, bgcolor); // Draw white sreen
    display.setTextColor(textcolor);        // Set display color
    display.update();                       // Full update (slow)
        
    if(String(displaymode) == "Logo + QR Code" || String(displaymode) == "Logo"){
        display.drawBitmap(gImage_Logo_OBP_400x300_sw, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, pixelcolor); // Draw start logo
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);   // Partial update (fast)
        delay(SHOW_TIME);                                // Logo show time
        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, bgcolor); // Draw white sreen
        if(refreshmode == true){
            display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, bgcolor); // Draw white sreen
            display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Needs partial update before full update to refresh the frame buffer
            display.update(); // Full update
        }
        else{
            display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)
        }
        if(String(displaymode) == "Logo + QR Code"){
            qrWiFi(systemname, wifipass, displaycolor);  // Show QR code for WiFi connection
            delay(SHOW_TIME);                            // Logo show time
            display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, bgcolor); // Draw white sreen
            display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Needs partial update before full update to refresh the frame buffer
            display.update(); // Full update
        }
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
    xTaskCreate(keyboardTask,"keyboard",2000,&allParameters,configMAX_PRIORITIES-1,NULL);


    // Task Loop
    //###############################

    // Configuration values for main loop
    String gpsFix = api->getConfig()->getConfigItem(api->getConfig()->flashLED,true)->asString();
    String gps = api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asString();
    String backlight = api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();
    String envsensor = api->getConfig()->getConfigItem(api->getConfig()->useEnvSensor,true)->asString();
    // refreshmode defined in init section
    // displaycolor defined in init section
    // textcolor defined in init section
    // pixelcolor defined in init section
    // bgcolor defined in init section

    // Boat values for main loop
    GwApi::BoatValue *hdop = boatValues.findValueOrCreate("HDOP");      // Load HDOP
    GwApi::BoatValue *date = boatValues.findValueOrCreate("GPSD");      // Load GpsDate
    GwApi::BoatValue *time = boatValues.findValueOrCreate("GPST");      // Load GpsTime

    // Internal sensor values
    double batteryVoltage = 0;

    LOG_DEBUG(GwLog::LOG,"obp60task: start mainloop");
    int pageNumber=0;
    int lastPage=pageNumber;
    commonData.data.actpage = pageNumber + 1;
    commonData.data.maxpage = numPages;
    commonData.time = boatValues.findValueOrCreate("GPST");      // Load GpsTime
    commonData.date = boatValues.findValueOrCreate("GPSD");      // Load GpsTime
    bool delayedDisplayUpdate = false;  // If select a new pages then make a delayed full display update
    long firststart = millis();     // First start
    long starttime0 = millis();     // Mainloop
    long starttime1 = millis();     // Full display refresh for the first 5 min (more often as normal)
    long starttime2 = millis();     // Full display refresh after 5 min
    long starttime3 = millis();     // Display update all 1s
    long starttime4 = millis();     // Delayed display update after 4s when select a new page
    long starttime5 = millis();     // Voltage update all 1s
    long starttime6 = millis();     // Environment sensor update all 1s

    while (true){
        delay(10);                  // Fixed the problem with NMEA0183 and GPS sentences
        Timer1.update();            // Update for Timer1
        Timer2.update();            // Update for Timer2
        if(millis() > starttime0 + 100){
            starttime0 = millis();

            // Send NMEA0183 GPS data on several bus systems all 1000ms
            if(String(gps) == "NEO-6M" || String(gps) == "NEO-M8N"){   // If config enabled
                if(gps_ready == true){
                    tNMEA0183Msg NMEA0183Msg;
                    while(NMEA0183.GetMessage(NMEA0183Msg)){
                        api->sendNMEA0183Message(NMEA0183Msg);
                    }
                }
            }

            // If GPS fix then LED off (HDOP)
            if(String(gpsFix) == "GPS Fix" && hdop->valid == true && int(hdop->value) <= 50){
                setPortPin(OBP_FLASH_LED, false);
            }
            // Ifmissing GPS fix then LED on
            if(String(gpsFix) == "GPS Fix" && ((hdop->valid == true && int(hdop->value) > 50) || hdop->valid == false)){
                setPortPin(OBP_FLASH_LED, true);
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
                            togglePortPin(OBP_BACKLIGHT_LED);
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
/*                                
                    // #9 or #10 Refresh display befor start a new page if reshresh is enabled
                    if(refreshmode == true && (keyboardMessage == 9 || keyboardMessage == 10)){
                        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
                        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Needs partial update before full update to refresh the frame buffer
                        display.update(); // Full update
                    }
*/                    
                    // #9 or #10 Refresh display after a new page after 4s waiting time and if refresh is disabled
                    if(refreshmode == true && (keyboardMessage == 9 || keyboardMessage == 10)){
                        starttime4 = millis();
                        delayedDisplayUpdate = true;
                    }                 
                }
                LOG_DEBUG(GwLog::LOG,"set pagenumber to %d",pageNumber);
            }

            // Full display update afer a new selected page and 4s wait time
            if(millis() > starttime4 + 4000 && delayedDisplayUpdate == true){
                display.update(); // Full update
                delayedDisplayUpdate = false;
            }

            // Subtask E-Ink full refresh all 1 min for the first 5min after power on or restart
            // This needs for a better display contrast after power on in cold or warm environments
            if(millis() < firststart + (5 * 60 * 1000) && millis() > starttime1 + (60 * 1000)){
                starttime1 = millis();
                LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh first 5 min");
                display.update(); // Full update
            }

            // Subtask E-Ink full refresh
            if(millis() > starttime2 + FULL_REFRESH_TIME * 1000){
                starttime2 = millis();
                LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh");
                display.update(); // Full update
            }

            // Send supplay voltage value
            if(millis() > starttime5 + 1000){
                starttime5 = millis();
                batteryVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // Vin = 1/20
                commonData.data.batteryVoltage = batteryVoltage;   // Data take over to page
                // Send to NMEA200 bus
                if(!isnan(batteryVoltage)){
                    SetN2kDCBatStatus(N2kMsg, 0, batteryVoltage, N2kDoubleNA, N2kDoubleNA, 1);
                    api->sendN2kMessage(N2kMsg);
                }
            }

            // Send data from environment sensor
            if(millis() > starttime6 + 1000){
                starttime6 = millis();
                unsigned char TempSource = 2;       // Inside temperature
                unsigned char PressureSource = 0;   // Atmospheric pressure
                unsigned char HumiditySource=0;     // Inside humidity
                LOG_DEBUG(GwLog::LOG,"Ready status BME280 %d", BME280_ready);
                if(envsensor == "BME280" && BME280_ready == true){
                    airtemperature = bme280.readTemperature();
                    commonData.data.airTemperature = airtemperature;   // Data take over to page
                    airpressure = bme280.readPressure()/100;
                    commonData.data.airPressure = airpressure;         // Data take over to page
                    airhumidity = bme280.readHumidity();
                    commonData.data.airHumidity = airhumidity;         // Data take over to page
                    // Send to NMEA200 bus
                    if(!isnan(airtemperature)){
                        SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(airtemperature), N2kDoubleNA);
                        api->sendN2kMessage(N2kMsg);
                    }
                    if(!isnan(airhumidity)){
                        SetN2kPGN130313(N2kMsg, 0, 0,(tN2kHumiditySource) HumiditySource, airhumidity, N2kDoubleNA);
                        api->sendN2kMessage(N2kMsg);
                    }
                    if(!isnan(airpressure)){
                        SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) mBarToPascal(PressureSource), airpressure);
                        api->sendN2kMessage(N2kMsg);
                    }
                }
                else if(envsensor == "BMP280" && BMP280_ready == true){
                    airtemperature = bmp280.readTemperature();
                    commonData.data.airTemperature = airtemperature;   // Data take over to page
                    airpressure  =bmp280.readPressure()/100;
                    commonData.data.airPressure = airpressure;         // Data take over to page
                    // Send to NMEA200 bus
                    if(!isnan(airtemperature)){
                        SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(airtemperature), N2kDoubleNA);
                        api->sendN2kMessage(N2kMsg);
                    }
                    if(!isnan(airpressure)){
                        SetN2kPGN130314(N2kMsg, 0, 0, (tN2kPressureSource) mBarToPascal(PressureSource), airpressure);
                        api->sendN2kMessage(N2kMsg);
                    }
                }
                else if(envsensor == "SHT21" && BME280_ready == true){
                    airhumidity = sht21.readCompensatedHumidity();
                    commonData.data.airHumidity = airhumidity;         // Data take over to page
                    airtemperature = sht21.readTemperature();
                    commonData.data.airTemperature = airtemperature;   // Data take over to page
                    // Send to NMEA200 bus
                    if(!isnan(airtemperature)){
                        SetN2kPGN130312(N2kMsg, 0, 0,(tN2kTempSource) TempSource, CToKelvin(airtemperature), N2kDoubleNA);
                        api->sendN2kMessage(N2kMsg);
                    }
                    if(!isnan(airhumidity)){
                        SetN2kPGN130313(N2kMsg, 0, 0,(tN2kHumiditySource) HumiditySource, airhumidity, N2kDoubleNA);
                        api->sendN2kMessage(N2kMsg);
                    }
                }                
            }

            // Refresh display data all 1s
            if(millis() > starttime3 + 1000){
                starttime3 = millis();
                //refresh data from api
                api->getBoatDataValues(boatValues.numValues,boatValues.allBoatValues);
                api->getStatus(commonData.status);

                // Show header if enabled
                display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, bgcolor);   // Clear display
                if (pages[pageNumber].description && pages[pageNumber].description->header){
                    //build some header and footer using commonData
                    displayHeader(commonData, hdop, date, time);
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