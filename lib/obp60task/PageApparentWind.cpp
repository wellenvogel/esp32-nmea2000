#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageApparentWind : public Page
{
int16_t lp = 80;                    // Pointer length

public:
    PageApparentWind(CommonData &common){
        commonData = &common;
        common.logger->logDebug(GwLog::LOG,"Instantiate PageApparentWind");
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

        // Code for keylock
        if(key == 11){
            commonData->keylock = !commonData->keylock;
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

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        // bool simulation = config->getBool(config->useSimuData);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);

        // Get boat values for AWS
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = bvalue1->getName().c_str();      // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        double value1 = bvalue1->value;                 // Value as double in SI unit
        // bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, *commonData).unit;        // Unit of value

        // Get boat values for AWD
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // First element in list (only one value by PageOneValue)
        String name2 = bvalue2->getName().c_str();      // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        double value2 = bvalue2->value;                 // Value as double in SI unit
        // bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, *commonData).unit;        // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageApparentWind, %s:%f,  %s:%f", name1.c_str(), value1, name2.c_str(), value2);

        // Draw page
        //***********************************************************

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        getdisplay().setTextColor(commonData->fgcolor);

        // Show values AWS
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 50);
        if(holdvalues == false){
            getdisplay().print(name1);                       // Value name
            getdisplay().print(": ");
            getdisplay().print(svalue1);                     // Value
            getdisplay().print(" ");
            getdisplay().print(unit1);                       // Unit
        }
        else{
            getdisplay().print(name1);                       // Value name
            getdisplay().print(": ");
            getdisplay().print(svalue1old);                  // Value old
            getdisplay().print(" ");
            getdisplay().print(unit1old);                    // Unit old
        }

        // Show values AWD
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 260);
        if(holdvalues == false){
            getdisplay().print(name2);                       // Value name
            getdisplay().print(": ");
            getdisplay().print(svalue2);                     // Value
            getdisplay().print(" ");
            getdisplay().print(unit2);                       // Unit
        }
        else{
            getdisplay().print(name2);                       // Value name
            getdisplay().print(": ");
            getdisplay().print(svalue2old);                  // Value old
            getdisplay().print(" ");
            getdisplay().print(unit2old);                    // Unit old
        }

        // Draw wind pointer
        static int16_t x0 = 200;    // Center point
        static int16_t y0 = 145;
        static int16_t x1 = x0;     // Start point for pointer
        static int16_t y1 = y0;
        static int16_t x2 = x0;     // End point for pointer
        static int16_t y2 = y0;

        //Draw instrument
        getdisplay().fillCircle(x0, y0, lp + 5, commonData->fgcolor);
        getdisplay().fillCircle(x0, y0, lp + 1, commonData->bgcolor);

        // Calculation end point of pointer
        value2 = value2 - 3.14 / 2;
        x1 = x0 + cos(value2) * lp * 0.6;
        y1 = y0 + sin(value2) * lp * 0.6;
        x2 = x0 + cos(value2) * lp;
        y2 = y0 + sin(value2) * lp;
        getdisplay().drawLine(x1, y1, x2, y2, commonData->fgcolor);

        // Key Layout
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(commonData->keylock == false){
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                getdisplay().setCursor(343, 290);
                getdisplay().print("[ILUM]");
            }
        }

        // Update display
        getdisplay().nextPage();     // Partial update (fast)

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
    "ApparentWind",     // Page name
    createPage,         // Action
    0,                  // Number of bus values depends on selection in Web configuration
    {"AWS","AWA"},      // Bus values we need in the page
    true                // Show display header on/off
);

#endif
