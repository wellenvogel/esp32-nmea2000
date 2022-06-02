#ifdef BOARD_NODEMCU32S_OBP60

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageClock : public Page
{
bool keylock = false;               // Keylock

public:
    PageClock(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageClock");
    }

    // Key functions
    virtual int handleKey(int key){
        // Keylock function
        if(key == 11){              // Code for keylock
            keylock = !keylock;     // Toggle keylock
            return 0;               // Commit the key
        }
        return key;
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData)
    {
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;

        static String svalue1old = "";
        static String unit1old = "";
        static String svalue2old = "";
        static String unit2old = "";
        static String svalue3old = "";
        static String svalue4old = "";

        double value1 = 0;
        double value2 = 0;

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
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
            value1 = 38160;                             // Simulation data for time value 11:36 in seconds
        }                                               // Other simulation data see OBP60Formater.cpp
        bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, commonData).unit;        // Unit of value
        if(valid1 == true){
            svalue1old = svalue1;   	                // Save old value
            unit1old = unit1;                           // Save old unit
        }

        // Get boat values for GPS date
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // First element in list (only one value by PageOneValue)
        String name2 = bvalue2->getName().c_str();      // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        value2 = bvalue2->value;                        // Value as double in SI unit
        bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, commonData).unit;        // Unit of value
        if(valid2 == true){
            svalue2old = svalue2;   	                // Save old value
            unit2old = unit2;                           // Save old unit
        }

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setPortPin(OBP_FLASH_LED, false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageClock, %s:%f,  %s:%f", name1, value1, name2, value2);

        // Draw page
        //***********************************************************

        // Set background color and text color
        int textcolor = GxEPD_BLACK;
        int pixelcolor = GxEPD_BLACK;
        int bgcolor = GxEPD_WHITE;
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
        // Clear display by call in obp60task.cpp in main loop

        // Show values GPS date
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(10, 65);
        if(holdvalues == false) display.print(svalue2); // Value
        else display.print(svalue2old);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(10, 95);
        display.print("Date");                          // Name

        // Horizintal separator left
        display.fillRect(0, 149, 60, 3, pixelcolor);

        // Show values GPS time
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(10, 250);
        if(holdvalues == false) display.print(svalue1); // Value
        else display.print(svalue1old);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(10, 220);
        display.print("Time");                          // Name

        // Show values sunrise
        String sunrise = "---";
        if(valid1 == true && valid2 == true){
            sunrise = String(commonData.sundata.sunriseHour) + ":" + String(commonData.sundata.sunriseMinute + 100).substring(1);
            svalue3old = sunrise;
        }

        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(335, 65);
        display.print(sunrise);                         // Value
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(335, 95);
        if(holdvalues == false) display.print(sunrise); // Value
        else display.print(svalue3old);

        // Horizintal separator right
        display.fillRect(340, 149, 80, 3, pixelcolor);

        // Show values sunset
        String sunset = "---";
        if(valid1 == true && valid2 == true){
            sunset = String(commonData.sundata.sunsetHour) + ":" +  String(commonData.sundata.sunsetMinute + 100).substring(1);
            svalue4old = sunset;
        }

        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(335, 250);
        if(holdvalues == false) display.print(sunset); // Value
        else display.print(svalue4old);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(335, 220);
        display.print("SunS");                          // Name

//*******************************************************************************************
        
        // Draw clock
        int rInstrument = 110;     // Radius of clock
        float pi = 3.141592;

        display.fillCircle(200, 150, rInstrument + 10, pixelcolor);    // Outer circle
        display.fillCircle(200, 150, rInstrument + 7, bgcolor);        // Outer circle     

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
            display.getTextBounds(ii, int(x), int(y), &x1, &y1, &w, &h); // Calc width of new string
            display.setCursor(x-w/2, y+h/2);
            if(i % 30 == 0){
                display.setFont(&Ubuntu_Bold12pt7b);
                display.print(ii);
            }

            // Draw sub scale with dots
            float sinx = 0;
            float cosx = 0;
             if(i % 6 == 0){
                float x1c = 200 + rInstrument*sin(i/180.0*pi);
                float y1c = 150 - rInstrument*cos(i/180.0*pi);
                display.fillCircle((int)x1c, (int)y1c, 2, pixelcolor);
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
                display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                        200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),pixelcolor);
                display.fillTriangle(200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),
                        200+(int)(cosx*xx2-sinx*yy2),150+(int)(sinx*xx2+cosx*yy2),pixelcolor);  
            }
        }

        // Print Unit in clock
        display.setTextColor(textcolor);
        if(holdvalues == false){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(175, 110);
            display.print(unit2);                       // Unit
        }
        else{
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(175, 110);
            display.print(unit2old);                    // Unit
        }

        // Clock values
        double hour = 0;
        double minute = 0;
        value1 = value1 + int(timezone*3600);
        if (value1 > 86400) {value1 = value1 - 86400;}
        if (value1 < 0) {value1 = value1 + 86400;}
        hour = (value1 / 3600.0);
        if(hour > 12) hour = hour - 12.0;
//        minute = (hour - int(hour)) * 3600.0 / 60.0;        // Analog minute pointer smoth moving
        minute = int((hour - int(hour)) * 3600.0 / 60.0);   // Jumping minute pointer from minute to minute
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
            display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),pixelcolor);   
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument * 0.5);
            float iy2 = -endwidth;
            display.fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),pixelcolor);
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
            display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),pixelcolor);   
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument - 15);
            float iy2 = -endwidth;
            display.fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),pixelcolor);
        }

        // Center circle
        display.fillCircle(200, 150, startwidth + 6, bgcolor);
        display.fillCircle(200, 150, startwidth + 4, pixelcolor);

//*******************************************************************************************
        // Key Layout
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            display.setCursor(130, 290);
            display.print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                display.setCursor(343, 290);
                display.print("[ILUM]");
            }
        }
        else{
            display.setCursor(130, 290);
            display.print(" [    Keylock active    ]");
        }

        // Update display
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)

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
    "Clock",         // Page name
    createPage,         // Action
    0,                  // Number of bus values depends on selection in Web configuration
    {"GPST", "GPSD"},    // Bus values we need in the page
    true                // Show display header on/off
);

#endif
