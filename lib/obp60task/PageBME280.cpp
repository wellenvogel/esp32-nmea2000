#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageBME280 : public Page
{
    bool keylock = false;               // Keylock

    public:
    PageBME280(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageBME280");
    }

    virtual int handleKey(int key){
        if(key == 11){                  // Code for keylock
            keylock = !keylock;         // Toggle keylock
            return 0;                   // Commit the key
        }
        return key;
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData){
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;

        double value1 = 0;
        double value2 = 0;
        double value3 = 0;
        String svalue1 = "";
        String svalue2 = "";
        String svalue3 = "";

        // Get config data
        String tempformat = config->getString(config->tempFormat);
        bool simulation = config->getBool(config->useSimuData);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);
        String useenvsensor = config->getString(config->useEnvSensor);
        
        // Get sensor values #1
        String name1 = "Temp";                          // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        if(simulation == false){
            value1 = commonData.data.airTemperature;    // Value as double in SI unit 
        }
        else{
            value1 = 23.0 + float(random(0, 10)) / 10.0;
        }
        // Display data when sensor activated
        if((String(useenvsensor) == "BME280") or (String(useenvsensor) == "BMP280")){
            svalue1 = String(value1, 1);                // Formatted value as string including unit conversion and switching decimal places
        }
        else{
            svalue1 = "---";
        }
        String unit1 = "Deg C";                         // Unit of value

        // Get sensor values #2
        String name2 = "Humid";                         // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        if(simulation == false){
            value2 = commonData.data.airHumidity;       // Value as double in SI unit 
        }
        else{
            value2 = 43 + float(random(0, 4));
        }
        // Display data when sensor activated
        if(String(useenvsensor) == "BME280"){
            svalue2 = String(value2, 0);                // Formatted value as string including unit conversion and switching decimal places
        }
        else{
            svalue2 = "---";
        }
        String unit2 = "%";                             // Unit of value

        // Get sensor values #3
        String name3 = "Press";                         // Value name
        name3 = name3.substring(0, 6);                  // String length limit for value name
         if(simulation == false){
            value3 = commonData.data.airPressure;       // Value as double in SI unit 
        }
        else{
            value3 = 1006 + float(random(0, 5));
        }
        // Display data when sensor activated
        if((String(useenvsensor) == "BME280") or (String(useenvsensor) == "BMP280")){
            svalue3 = String(value3 / 100, 1);          // Formatted value as string including unit conversion and switching decimal places
        }
        else{
            svalue3 = "---";
        }
        String unit3 = "hPa";                          // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageBME280, %s: %f, %s: %f, %s: %f", name1.c_str(), value1, name2.c_str(), value2, name3.c_str(), value3);

        // Draw page
        //***********************************************************

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        getdisplay().setTextColor(commonData.fgcolor);

        // ############### Value 1 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 55);
        getdisplay().print(name1);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 90);
        getdisplay().print(unit1);                           // Unit

        // Switch font if format for any values
        getdisplay().setFont(&DSEG7Classic_BoldItalic30pt7b);
        getdisplay().setCursor(180, 90);

        // Show bus data
        getdisplay().print(svalue1);                         // Real value as formated string

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        getdisplay().fillRect(0, 105, 400, 3, commonData.fgcolor);

        // ############### Value 2 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 145);
        getdisplay().print(name2);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 180);
        getdisplay().print(unit2);                           // Unit

        // Switch font if format for any values
        getdisplay().setFont(&DSEG7Classic_BoldItalic30pt7b);
        getdisplay().setCursor(180, 180);

        // Show bus data
        getdisplay().print(svalue2);                         // Real value as formated string

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        getdisplay().fillRect(0, 195, 400, 3, commonData.fgcolor);

        // ############### Value 3 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 235);
        getdisplay().print(name3);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 270);
        getdisplay().print(unit3);                           // Unit

        // Switch font if format for any values
        getdisplay().setFont(&DSEG7Classic_BoldItalic30pt7b);
        getdisplay().setCursor(140, 270);

        // Show bus data
        getdisplay().print(svalue3);                         // Real value as formated string

        // ############### Key Layout ################

        // Key Layout
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
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
    return new PageBME280(common);
}/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageBME280(
    "BME280",  // Page name
    createPage,     // Action
    0,              // Number of bus values depends on selection in Web configuration
    {},             // Names of bus values undepends on selection in Web configuration (refer GwBoatData.h)
    true            // Show display header on/off
);

#endif
