#include "Pagedata.h"
#include "OBP60ExtensionPort.h"

class PageApparentWind : public Page
{
bool keylock = false;               // Keylock
int16_t lp = 80;                    // Pointer length

public:
    PageApparentWind(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageApparentWind");
    }

    // Key functions
    virtual int handleKey(int key){
        // Reduce instrument size
        if(key == 2){               // Code for reduce
            lp = lp - 10;
            if(lp < 10){
                lp = 10;
            }
            return 0;               // Commit the key
        }

        // Enlarge instrument size
        if(key == 5){               // Code for enlarge
            lp = lp + 10;
            if(lp > 80){
                lp = 80;
            }
            return 0;               // Commit the key
        }

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

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);

        // Get boat values for AWS
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = bvalue1->getName().c_str();      // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        double value1 = bvalue1->value;                 // Value as double in SI unit
        bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, commonData).unit;        // Unit of value

        // Get boat values for AWD
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // First element in list (only one value by PageOneValue)
        String name2 = bvalue2->getName().c_str();      // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        double value2 = bvalue2->value;                 // Value as double in SI unit
        bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, commonData).unit;        // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setPortPin(OBP_FLASH_LED, false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageApparentWind, %s:%f,  %s:%f", name1, value1, name2, value2);

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

        // Show values AWS
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 50);
        if(holdvalues == false){
            display.print(name1);                       // Value name
            display.print(": ");
            display.print(svalue1);                     // Value
            display.print(" ");
            display.print(unit1);                       // Unit
        }
        else{
            display.print(name1);                       // Value name
            display.print(": ");
            display.print(svalue1old);                  // Value old
            display.print(" ");
            display.print(unit1old);                    // Unit old
        }

        // Show values AWD
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 260);
        if(holdvalues == false){
            display.print(name2);                       // Value name
            display.print(": ");
            display.print(svalue2);                     // Value
            display.print(" ");
            display.print(unit2);                       // Unit
        }
        else{
            display.print(name2);                       // Value name
            display.print(": ");
            display.print(svalue2old);                  // Value old
            display.print(" ");
            display.print(unit2old);                    // Unit old
        }

        // Draw wind pointer
        static int16_t x0 = 200;    // Center point
        static int16_t y0 = 145;
        static int16_t x1 = x0;     // Start point for pointer
        static int16_t y1 = y0;
        static int16_t x2 = x0;     // End point for pointer
        static int16_t y2 = y0;

        //Draw instrument
        display.fillCircle(x0, y0, lp + 5, pixelcolor); // Black circle
        display.fillCircle(x0, y0, lp + 1, bgcolor);    // White circle

        // Calculation end point of pointer
        value2 = value2 - 3.14 / 2;
        x1 = x0 + cos(value2) * lp * 0.6;
        y1 = y0 + sin(value2) * lp * 0.6;
        x2 = x0 + cos(value2) * lp;
        y2 = y0 + sin(value2) * lp;

        display.drawLine(x1, y1, x2, y2, pixelcolor);

        // Key Layout
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(115, 290);
        if(keylock == false){
            display.print(" [  <<<<<<      >>>>>>  ]");
            if(String(backlightMode) == "Control by Key"){              // Key for illumination
                display.setCursor(343, 290);
                display.print("[ILUM]");
            }
        }
        else{
            display.print(" [    Keylock active    ]");
        }

        // Update display
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)

    };
};

static Page *createPage(CommonData &common){
    return new PageApparentWind(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageApparentWind(
    "apparentWind",     // Page name
    createPage,         // Action
    0,                  // Number of bus values depends on selection in Web configuration
    {"AWS","AWA"},      // Bus values we need in the page
    true                // Show display header on/off
);