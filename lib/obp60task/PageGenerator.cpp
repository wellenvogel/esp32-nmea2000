#ifdef BOARD_NODEMCU32S_OBP60

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageGenerator : public Page
{
bool init = false;                  // Marker for init done
bool keylock = false;               // Keylock
int genPercentage = 0;              // Generator power level

public:
    PageGenerator(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageGenerator");
    }
    virtual int handleKey(int key){
        // Code for keylock
        if(key == 11){
            keylock = !keylock;         // Toggle keylock
            return 0;                   // Commit the key
        }
        return key;
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData)
    {
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;
        
        // Get config data
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String batVoltage = config->getString(config->batteryVoltage);
        int generatorMaxPower = config->getInt(config->genPower);
        String backlightMode = config->getString(config->backlight);
        String powerSensor = config->getString(config->usePowSensor3);

        double value1 = 0;  // Generator voltage
        double value2 = 0;  // Generator current
        double value3 = 0;  // Generator power consumption

        // Get values
        value1 = commonData.data.generatorVoltage;          // Live data
        value2 = commonData.data.generatorCurrent;
        value3 = commonData.data.generatorPower;
        genPercentage = value3 / generatorMaxPower * 100;   // Power level calculation
        if(genPercentage < 0){      // Limiting values
            genPercentage = 0;
        }
        if(genPercentage > 99){
            genPercentage = 99;
        }
        bool valid1 = true;

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setPortPin(OBP_FLASH_LED, false); 
        }
        
        // Logging voltage value
        if (value1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageGenerator, V:%f C:%f P:%f", value1, value2, value3);

        // Draw page
        //***********************************************************

        // Clear display, set background color and text color
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
        // Clear display in obp60task.cpp in main loop

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(10, 65);
        display.print("Generator");

        // Show voltage type
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(10, 140);
        int bvoltage = 0;
        if(String(batVoltage) == "12V") bvoltage = 12;
        else bvoltage = 24;
        display.print(bvoltage);
        display.setFont(&Ubuntu_Bold16pt7b);
        display.print("V");

        // Show generator power level
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(10, 200);
        if(generatorMaxPower <= 999) display.print(generatorMaxPower, 0);
        if(generatorMaxPower > 999) display.print(float(generatorMaxPower/1000.0), 1);
        display.setFont(&Ubuntu_Bold16pt7b);
        if(generatorMaxPower <= 999) display.print("W");
        if(generatorMaxPower > 999) display.print("kw");

        // Show info
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(10, 235);
        display.print("Installed");
        display.setCursor(10, 255);
        display.print("Gen. Power");

        // Show generator icon
        batteryGraphic(150, 45, genPercentage, pixelcolor, bgcolor);

        // Show average settings
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(150, 145);
        display.print("Avg: 1s");

        // Show power level in percent
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(150, 200);
        display.print(genPercentage);
        display.setFont(&Ubuntu_Bold16pt7b);
        display.print("%");

        // Show sensor type info
        String i2cAddr = "";
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(270, 60);
        if(powerSensor == "off") display.print("Internal");
        if(powerSensor == "INA219"){
            display.print("INA219");
        }
        if(powerSensor == "INA226"){
            display.print("INA226");
            i2cAddr = " (0x" + String(INA226_I2C_ADDR3, HEX) + ")";
        }
        display.print(i2cAddr);
        display.setCursor(270, 80);
        display.print("Sensor Modul");

        // Reading bus data or using simulation data
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(260, 140);
        if(simulation == true){
            if(batVoltage == "12V"){
                value1 = 12.0;
            }
            if(batVoltage == "24V"){
                value1 = 24.0;
            }
            value1 += float(random(0, 5)) / 10;         // Simulation data
            display.print(value1,1);
        }
        else{
            // Check for valid real data, display also if hold values activated
            if(valid1 == true || holdvalues == true){
                // Resolution switching
                if(value1 <= 9.9) display.print(value1, 2);
                if(value1 > 9.9 && value1 <= 99.9)display.print(value1, 1);
                if(value1 > 99.9) display.print(value1, 0);
            }
            else{
            display.print("---");                       // Missing bus data
            }
        }
        display.setFont(&Ubuntu_Bold16pt7b);
        display.print("V");

        // Show actual current in A
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(260, 200);
        if((powerSensor == "INA219" || powerSensor == "INA226") && simulation == false){
            if(value2 <= 9.9) display.print(value2, 2);
            if(value2 > 9.9 && value2 <= 99.9)display.print(value2, 1);
            if(value2 > 99.9) display.print(value2, 0);
        }
        else  display.print("---");
        display.setFont(&Ubuntu_Bold16pt7b);
        display.print("A");

        // Show actual consumption in W
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(260, 260);
        if((powerSensor == "INA219" || powerSensor == "INA226") && simulation == false){
            if(value3 <= 9.9) display.print(value3, 2);
            if(value3 > 9.9 && value3 <= 99.9)display.print(value3, 1);
            if(value3 > 99.9) display.print(value3, 0);
        }
        else  display.print("---");
        display.setFont(&Ubuntu_Bold16pt7b);
        display.print("W");

        // Key Layout
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            display.setCursor(130, 290);
            display.print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){              // Key for illumination
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
    return new PageGenerator(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageGenerator(
    "Generator",    // Name of page
    createPage,     // Action
    0,              // Number of bus values depends on selection in Web configuration
    {},             // Names of bus values undepends on selection in Web configuration (refer GwBoatData.h)
    true            // Show display header on/off
);

#endif