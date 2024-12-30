#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

/*
 * TODO mode: race timer: keys
 *   - prepare: set countdown to 5min
 *     reset: abort current countdown and start over with 5min preparation
 *   - 5min: key press
 *   - 4min: key press to sync
 *   - 1min: buzzer signal
 *   - start: buzzer signal for start
 *
 */

class PageClock : public Page
{
bool simulation = false;
int simtime;
bool keylock = false;
char source = 'R';  // time source (R)TC | (G)PS
char mode = 'A';    // display mode (A)nalog | (D)igital | race (T)imer
char tz = 'L';      // time zone (L)ocal | (U)TC

    public:
    PageClock(CommonData &common){
        commonData = &common;
        common.logger->logDebug(GwLog::LOG,"Instantiate PageClock");
        simulation = common.config->getBool(common.config->useSimuData);
        simtime = 38160; // time value 11:36
    }

    // Key functions
    virtual int handleKey(int key){
        // Time source
        if (key == 1) {
            if (source == 'G') {
                source = 'R';
            } else {
                source = 'G';
            }
            return 0;
        }
        if (key == 2) {
            if (mode == 'A') {
                mode = 'D';
            } else if (mode == 'D') {
                mode = 'T';
            } else {
                mode = 'A';
            }
            return 0;
        }
        // Time zone: Local / UTC
        if (key == 5) {
            if (tz == 'L') {
                tz = 'U';
            } else {
                tz = 'L';
            }
            return 0;
        }

        // Keylock function
        if(key == 11){              // Code for keylock
            keylock = !keylock;     // Toggle keylock
            return 0;               // Commit the key
        }
        return key;
    }

    virtual void displayPage(PageData &pageData)
    {
        GwConfigHandler *config = commonData->config;
        GwLog *logger = commonData->logger;

        static String svalue1old = "";
        static String unit1old = "";
        static String svalue2old = "";
        static String unit2old = "";
        static String svalue3old = "";
        static String unit3old = "";

        static String svalue5old = "";
        static String svalue6old = "";

        double value1 = 0;
        double value2 = 0;
        double value3 = 0;

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        String dateformat = config->getString(config->dateFormat);
        bool simulation = config->getBool(config->useSimuData);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);
        String stimezone = config->getString(config->timeZone);
        double timezone = stimezone.toDouble();

        // Get boat values for GPS time
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = bvalue1->getName().c_str();      // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        if(simulation == false){
            value1 = bvalue1->value;                    // Value as double in SI unit
        }
        else{
            value1 = simtime++;                         // Simulation data for time value 11:36 in seconds
        }                                               // Other simulation data see OBP60Formater.cpp
        bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, *commonData).unit;        // Unit of value
        if(valid1 == true){
            svalue1old = svalue1;   	                // Save old value
            unit1old = unit1;                           // Save old unit
        }

        // Get boat values for GPS date
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // Second element in list (only one value by PageOneValue)
        String name2 = bvalue2->getName().c_str();      // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        value2 = bvalue2->value;                        // Value as double in SI unit
        bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, *commonData).unit;        // Unit of value
        if(valid2 == true){
            svalue2old = svalue2;   	                // Save old value
            unit2old = unit2;                           // Save old unit
        }

        // Get boat values for HDOP date
        GwApi::BoatValue *bvalue3 = pageData.values[2]; // Third element in list (only one value by PageOneValue)
        String name3 = bvalue3->getName().c_str();      // Value name
        name3 = name3.substring(0, 6);                  // String length limit for value name
        value3 = bvalue3->value;                        // Value as double in SI unit
        bool valid3 = bvalue3->valid;                   // Valid information 
        String svalue3 = formatValue(bvalue3, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit3 = formatValue(bvalue3, *commonData).unit;        // Unit of value
        if(valid3 == true){
            svalue3old = svalue3;   	                // Save old value
            unit3old = unit3;                           // Save old unit
        }

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false);
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageClock, %s:%f,  %s:%f", name1.c_str(), value1, name2.c_str(), value2);

        // Draw page
        //***********************************************************

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        getdisplay().setTextColor(commonData->fgcolor);

        time_t tv = mktime(&commonData.data.rtcTime) + timezone * 3600;
        struct tm *local_tm = localtime(&tv);

        // Show values GPS date
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(10, 65);
        if (holdvalues == false) {
            if (source == 'G') {
                 // GPS value
                 getdisplay().print(svalue2);
            } else {
                // RTC value
                 if (tz == 'L') {
                     getdisplay().print(formatDate(dateformat, local_tm->tm_year + 1900, local_tm->tm_mon + 1, local_tm->tm_mday));
                 }
                 else {
                     getdisplay().print(formatDate(dateformat, commonData.data.rtcTime.tm_year + 1900, commonData.data.rtcTime.tm_mon + 1, commonData.data.rtcTime.tm_mday));
                 }
            }
        } else {
            getdisplay().print(svalue2old);
        }
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(10, 95);
        getdisplay().print("Date");                          // Name

        // Horizintal separator left
        getdisplay().fillRect(0, 149, 60, 3, commonData->fgcolor);

        // Show values GPS time
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(10, 250);
        if (holdvalues == false) {
            if (source == 'G') {
                getdisplay().print(svalue1); // Value
            }
            else {
                 if (tz == 'L') {
                      getdisplay().print(formatTime('s', local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec));
                 }
                 else {
                      getdisplay().print(formatTime('s', commonData.data.rtcTime.tm_hour, commonData.data.rtcTime.tm_min, commonData.data.rtcTime.tm_sec));
                 }
            }
        }
        else {
             getdisplay().print(svalue1old);
        }
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(10, 220);
        getdisplay().print("Time");                          // Name

        // Show values sunrise
        String sunrise = "---";
        if(valid1 == true && valid2 == true && valid3 == true){
            sunrise = String(commonData->sundata.sunriseHour) + ":" + String(commonData->sundata.sunriseMinute + 100).substring(1);
            svalue5old = sunrise;
        } else if (simulation) {
            sunrise = String("06:42");
        }

        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(335, 65);
        if(holdvalues == false) getdisplay().print(sunrise); // Value
        else getdisplay().print(svalue5old);
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(335, 95);
        getdisplay().print("SunR");                          // Name

        // Horizintal separator right
        getdisplay().fillRect(340, 149, 80, 3, commonData->fgcolor);

        // Show values sunset
        String sunset = "---";
        if(valid1 == true && valid2 == true && valid3 == true){
            sunset = String(commonData->sundata.sunsetHour) + ":" +  String(commonData->sundata.sunsetMinute + 100).substring(1);
            svalue6old = sunset;
        } else if (simulation) {
            sunset = String("21:03");
        }

        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(335, 250);
        if(holdvalues == false) getdisplay().print(sunset);  // Value
        else getdisplay().print(svalue6old);
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(335, 220);
        getdisplay().print("SunS");                          // Name

//*******************************************************************************************

        // Draw clock
        int rInstrument = 110;     // Radius of clock
        float pi = 3.141592;

        getdisplay().fillCircle(200, 150, rInstrument + 10, commonData->fgcolor);    // Outer circle
        getdisplay().fillCircle(200, 150, rInstrument + 7, commonData->bgcolor);     // Outer circle

        for(int i=0; i<360; i=i+1)
        {
            // Scaling values
            float x = 200 + (rInstrument-30)*sin(i/180.0*pi);  //  x-coordinate dots
            float y = 150 - (rInstrument-30)*cos(i/180.0*pi);  //  y-coordinate cots 
            const char *ii = "";
            switch (i)
            {
            case 0: ii="12"; break;
            case 30 : ii=""; break;
            case 60 : ii=""; break;
            case 90 : ii="3"; break;
            case 120 : ii=""; break;
            case 150 : ii=""; break;
            case 180 : ii="6"; break;
            case 210 : ii=""; break;
            case 240 : ii=""; break;
            case 270 : ii="9"; break;
            case 300 : ii=""; break;
            case 330 : ii=""; break;
            default: break;
            }

            // Print text centered on position x, y
            int16_t x1, y1;     // Return values of getTextBounds
            uint16_t w, h;      // Return values of getTextBounds
            getdisplay().getTextBounds(ii, int(x), int(y), &x1, &y1, &w, &h); // Calc width of new string
            getdisplay().setCursor(x-w/2, y+h/2);
            if(i % 30 == 0){
                getdisplay().setFont(&Ubuntu_Bold12pt7b);
                getdisplay().print(ii);
            }

            // Draw sub scale with dots
            float sinx = 0;
            float cosx = 0;
             if(i % 6 == 0){
                float x1c = 200 + rInstrument*sin(i/180.0*pi);
                float y1c = 150 - rInstrument*cos(i/180.0*pi);
                getdisplay().fillCircle((int)x1c, (int)y1c, 2, commonData->fgcolor);
                sinx=sin(i/180.0*pi);
                cosx=cos(i/180.0*pi);
             }

            // Draw sub scale with lines (two triangles)
            if(i % 30 == 0){
                float dx=2;   // Line thickness = 2*dx+1
                float xx1 = -dx;
                float xx2 = +dx;
                float yy1 =  -(rInstrument-10);
                float yy2 =  -(rInstrument+10);
                getdisplay().fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                        200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),commonData->fgcolor);
                getdisplay().fillTriangle(200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),
                        200+(int)(cosx*xx2-sinx*yy2),150+(int)(sinx*xx2+cosx*yy2),commonData->fgcolor);
            }
        }

        // Print Unit in clock
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(175, 110);
        if(holdvalues == false){
            if (tz == 'L') {
                getdisplay().print(unit2);                       // Unit
            } else {
                getdisplay().print("UTC");
            }
        }
        else{
            getdisplay().print(unit2old);                    // Unit
        }

        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(185, 190);
        if (source == 'G') {
            getdisplay().print("GPS");
        } else {
            getdisplay().print("RTC");
        }

        // Clock values
        double hour = 0;
        double minute = 0;
        if (source == 'R') {
            if (tz == 'L') {
                time_t tv = mktime(&commonData.data.rtcTime) + timezone * 3600;
                struct tm *local_tm = localtime(&tv);
                minute = local_tm->tm_min;
                hour = local_tm->tm_hour;
            } else {
                minute = commonData.data.rtcTime.tm_min;
                hour = commonData.data.rtcTime.tm_hour;
            }
            hour += minute / 60;
        } else {
            if (tz == 'L') {
                value1 += int(timezone*3600);
            }
            if (value1 > 86400) {value1 = value1 - 86400;}
            if (value1 < 0) {value1 = value1 + 86400;}
            hour = (value1 / 3600.0);
    //        minute = (hour - int(hour)) * 3600.0 / 60.0;        // Analog minute pointer smooth moving
            minute = int((hour - int(hour)) * 3600.0 / 60.0);   // Jumping minute pointer from minute to minute
        }
        if (hour > 12) {
            hour -= 12.0;
        }
        LOG_DEBUG(GwLog::DEBUG,"... PageClock, value1: %f hour: %f minute:%f", value1, hour, minute);

        // Draw hour pointer
        float startwidth = 8;       // Start width of pointer
        if(valid1 == true || holdvalues == true || simulation == true){
            float sinx=sin(hour * 30.0 * pi / 180);     // Hour
            float cosx=cos(hour * 30.0 * pi / 180);
            // Normal pointer
            // Pointer as triangle with center base 2*width
            float xx1 = -startwidth;
            float xx2 = startwidth;
            float yy1 = -startwidth;
            float yy2 = -(rInstrument * 0.5);
            getdisplay().fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),commonData->fgcolor);
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument * 0.5);
            float iy2 = -endwidth;
            getdisplay().fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),commonData->fgcolor);
        }

        // Draw minute pointer
        startwidth = 8;       // Start width of pointer
        if(valid1 == true || holdvalues == true || simulation == true){
            float sinx=sin(minute * 6.0 * pi / 180);     // Minute
            float cosx=cos(minute * 6.0 * pi / 180);
            // Normal pointer
            // Pointer as triangle with center base 2*width
            float xx1 = -startwidth;
            float xx2 = startwidth;
            float yy1 = -startwidth;
            float yy2 = -(rInstrument - 15);
            getdisplay().fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),commonData->fgcolor);
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument - 15);
            float iy2 = -endwidth;
            getdisplay().fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),commonData->fgcolor);
        }

        // Center circle
        getdisplay().fillCircle(200, 150, startwidth + 6, commonData->bgcolor);
        getdisplay().fillCircle(200, 150, startwidth + 4, commonData->fgcolor);

//*******************************************************************************************
        // Key Layout
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            getdisplay().setCursor(10, 290);
            getdisplay().print("[SRC]");
            getdisplay().setCursor(60, 290);
            getdisplay().print("[MODE]");
            getdisplay().setCursor(293, 290);
            getdisplay().print("[TZ]");
            getdisplay().setCursor(130, 290);
            getdisplay().print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                getdisplay().setCursor(343, 290);
                getdisplay().print("[ILUM]");
            }
        }
        else{
            getdisplay().setCursor(130, 290);
            getdisplay().print(" [    Keylock active    ]");
        }

        // Update display
        getdisplay().nextPage();    // Partial update (fast)

    };
};

static Page *createPage(CommonData &common){
    return new PageClock(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageClock(
    "Clock",            // Page name
    createPage,         // Action
    0,                  // Number of bus values depends on selection in Web configuration
    {"GPST", "GPSD", "HDOP"},   // Bus values we need in the page
    true                // Show display header on/off
);

#endif
