#ifdef BOARD_NODEMCU32S_OBP60

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageBME280 : public Page
{
    bool keylock = false;               // Keylock

    public:
    PageBME280(CommonData &comon){
        comon.logger->logDebug(GwLog::LOG,"Show PageThreeValue");
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
        String displaycolor = config->getString(config->displaycolor);
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
        if(String(useenvsensor) == "BME280"){
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
        if(String(useenvsensor) == "BME280"){
            svalue3 = String(value3, 0);                // Formatted value as string including unit conversion and switching decimal places
        }
        else{
            svalue3 = "---";
        }
        String unit3 = "hPa";                          // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setPortPin(OBP_FLASH_LED, false); 
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageBME280, %s: %f, %s: %f, %s: %f", name1, value1, name2, value2, name3, value3);

        // Draw page
        //***********************************************************

        // Set background color and text color
        int textcolor = GxEPD_BLACK;
        int pixelcolor = GxEPD_BLACK;
        if(displaycolor == "Normal"){
            textcolor = GxEPD_BLACK;
            pixelcolor = GxEPD_BLACK;
        }
        else{
            textcolor = GxEPD_WHITE;
            pixelcolor = GxEPD_WHITE;
        }
        // Clear display by call in obp60task.cpp in main loop

        // ############### Value 1 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 55);
        display.print(name1);                           // Page name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(20, 90);
        display.print(unit1);                           // Unit

        // Switch font if format for any values
        display.setFont(&DSEG7Classic_BoldItalic30pt7b);
        display.setCursor(180, 90);

        // Show bus data
        display.print(svalue1);                         // Real value as formated string

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        display.fillRect(0, 105, 400, 3, pixelcolor);

        // ############### Value 2 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 145);
        display.print(name2);                           // Page name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(20, 180);
        display.print(unit2);                           // Unit

        // Switch font if format for any values
        display.setFont(&DSEG7Classic_BoldItalic30pt7b);
        display.setCursor(180, 180);

        // Show bus data
        display.print(svalue2);                         // Real value as formated string

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        display.fillRect(0, 195, 400, 3, pixelcolor);

        // ############### Value 3 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 235);
        display.print(name3);                           // Page name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(20, 270);
        display.print(unit3);                           // Unit

        // Switch font if format for any values
        display.setFont(&DSEG7Classic_BoldItalic30pt7b);
        display.setCursor(180, 270);

        // Show bus data
        display.print(svalue3);                         // Real value as formated string

        // ############### Key Layout ################

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