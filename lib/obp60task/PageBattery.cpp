#ifdef BOARD_NODEMCU32S_OBP60

#include "Pagedata.h"
#include "OBP60ExtensionPort.h"

class PageBattery : public Page
{
    bool keylock = false;               // Keylock

    public:
    PageBattery(CommonData &comon){
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

        // Old values for hold function
        double value1 = 0;
        static String svalue1old = "";
        static String unit1old = "";
        double value2 = 0;
        static String svalue2old = "";
        static String unit2old = "";
        double value3 = 0;
        static String svalue3old = "";
        static String unit3old = "";

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        // bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);
        String powsensor1 = config->getString(config->usePowSensor1);
        bool simulation = config->getBool(config->useSimuData);
        
        // Get voltage value
        String name1 = "BatVolt";                       // Value name
        if(String(powsensor1) == "INA219" || String(powsensor1) == "INA226"){
            value1 = commonData.data.batteryVoltage;    // Value as double in SI unit
        }
        else{
            if(simulation == false){
                value1 = 0;                             // No sensor data
            }
            else{
                value1 = 12 + float(random(0, 5)) / 10; // Simulation data
            }
        }
        String svalue1 = String(value1);                // Formatted value as string including unit conversion and switching decimal places
        String unit1 = "V";                             // Unit of value

        // Get current value
        String name2 = "BatCurr";                       // Value name
        if(String(powsensor1) == "INA219" || String(powsensor1) == "INA226"){
            value2 = commonData.data.batteryCurrent;    // Value as double in SI unit
        }
        else{
            if(simulation == false){
                value2 = 0;                             // No sensor data
            }
            else{
                value2 = 8 + float(random(0, 10)) / 10; // Simulation data
            }
        }
        String svalue2 = String(value2);                // Formatted value as string including unit conversion and switching decimal places
        String unit2 = "A";                             // Unit of value

        // Get power value
        String name3 = "BatPow";                         // Value name
        if(String(powsensor1) == "INA219" || String(powsensor1) == "INA226"){
            value3 = commonData.data.batteryPower;       // Value as double in SI unit
        }
        else{
            if(simulation == false){
                value3 = 0;                             // No sensor data
            }
            else{
                value3 = value1 * value2;               // Simulation data
            }
        }
        String svalue3 = String(value3);                // Formatted value as string including unit conversion and switching decimal places
        String unit3 = "W";                             // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setPortPin(OBP_FLASH_LED, false); 
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageBattery, %s: %f, %s: %f, %s: %f", name1, value1, name2, value2, name3, value3);

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

        // ############### Value 1 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 55);
        display.print(name1);                           // Value name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(20, 90);
        display.print(unit1);                           // Unit


        // Show value
        display.setFont(&DSEG7Classic_BoldItalic30pt7b);
        display.setCursor(180, 90);

        // Show bus data
        display.print(value1,2);                        // Real value as formated string

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        display.fillRect(0, 105, 400, 3, pixelcolor);

        // ############### Value 2 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 145);
        display.print(name2);                           // Value name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(20, 180);
        display.print(unit2);                           // Unit

        // Show value
        display.setFont(&DSEG7Classic_BoldItalic30pt7b);
        display.setCursor(180, 180);

        // Show bus data
        display.print(value2,1);                        // Real value as formated string

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        display.fillRect(0, 195, 400, 3, pixelcolor);

        // ############### Value 3 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(20, 235);
        display.print(name3);                           // Value name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(20, 270);
        display.print(unit3);                           // Unit

        // Show value
        display.setFont(&DSEG7Classic_BoldItalic30pt7b);
        display.setCursor(180, 270);

        // Show bus data
        display.print(value3,1);                        // Real value as formated string


        // ############### Key Layout ################

        // Key Layout
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(130, 290);
        if(keylock == false){
            display.print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
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
    return new PageBattery(common);
}/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageBattery(
    "Battery",      // Page name
    createPage,     // Action
    0,              // Number of bus values depends on selection in Web configuration
    {},             // Names of bus values undepends on selection in Web configuration (refer GwBoatData.h)
    true            // Show display header on/off
);

#endif