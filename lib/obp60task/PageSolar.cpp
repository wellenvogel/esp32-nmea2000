#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageSolar : public Page
{
bool init = false;                  // Marker for init done
bool keylock = false;               // Keylock
int solPercentage = 0;              // Solar power level

public:
    PageSolar(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageSolar");
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
        int solarMaxPower = config->getInt(config->solarPower);
        String backlightMode = config->getString(config->backlight);
        String powerSensor = config->getString(config->usePowSensor2);

        double value1 = 0;  // Solar voltage
        double value2 = 0;  // Solar current
        double value3 = 0;  // Solar power consumption

        // Get values
        value1 = commonData.data.solarVoltage;        // Live data
        value2 = commonData.data.solarCurrent;
        value3 = commonData.data.solarPower;
        solPercentage = value3 / solarMaxPower * 100;   // Power level calculation
        if(solPercentage < 0){      // Limiting values
            solPercentage = 0;
        }
        if(solPercentage > 99){
            solPercentage = 99;
        }
        bool valid1 = true;

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }
        
        // Logging voltage value
        if (value1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageSolar, V:%f C:%f P:%f", value1, value2, value3);

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
        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update
        getdisplay().fillScreen(bgcolor);    // Clear display

        // Show name
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(10, 65);
        getdisplay().print("Solar");

        // Show voltage type
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(10, 140);
        int bvoltage = 0;
        if(String(batVoltage) == "12V") bvoltage = 12;
        else bvoltage = 24;
        getdisplay().print(bvoltage);
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().print("V");

        // Show solar power level
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(10, 200);
        if(solarMaxPower <= 999) getdisplay().print(solarMaxPower, 0);
        if(solarMaxPower > 999) getdisplay().print(float(solarMaxPower/1000.0), 1);
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        if(solarMaxPower <= 999) getdisplay().print("W");
        if(solarMaxPower > 999) getdisplay().print("kw");

        // Show info
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(10, 235);
        getdisplay().print("Installed");
        getdisplay().setCursor(10, 255);
        getdisplay().print("Power");

        // Show solar icon
        batteryGraphic(150, 45, solPercentage, pixelcolor, bgcolor);

        // Show average settings
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(150, 145);
        getdisplay().print("Avg: 1s");

        // Show power level in percent
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(150, 200);
        getdisplay().print(solPercentage);
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().print("%");

        // Show sensor type info
        String i2cAddr = "";
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(270, 60);
        if(powerSensor == "off") getdisplay().print("Internal");
        if(powerSensor == "INA219"){
            getdisplay().print("INA219");
        }
        if(powerSensor == "INA226"){
            getdisplay().print("INA226");
            i2cAddr = " (0x" + String(INA226_I2C_ADDR2, HEX) + ")";
        }
        getdisplay().print(i2cAddr);
        getdisplay().setCursor(270, 80);
        getdisplay().print("Sensor Modul");

        // Reading bus data or using simulation data
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(260, 140);
        if(simulation == true){
            if(batVoltage == "12V"){
                value1 = 12.0;
            }
            if(batVoltage == "24V"){
                value1 = 24.0;
            }
            value1 += float(random(0, 5)) / 10;         // Simulation data
            getdisplay().print(value1,1);
        }
        else{
            // Check for valid real data, display also if hold values activated
            if(valid1 == true || holdvalues == true){
                // Resolution switching
                if(value1 <= 9.9) getdisplay().print(value1, 2);
                if(value1 > 9.9 && value1 <= 99.9)getdisplay().print(value1, 1);
                if(value1 > 99.9) getdisplay().print(value1, 0);
            }
            else{
            getdisplay().print("---");                       // Missing bus data
            }
        }
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().print("V");

        // Show actual current in A
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(260, 200);
        if((powerSensor == "INA219" || powerSensor == "INA226") && simulation == false){
            if(value2 <= 9.9) getdisplay().print(value2, 2);
            if(value2 > 9.9 && value2 <= 99.9)getdisplay().print(value2, 1);
            if(value2 > 99.9) getdisplay().print(value2, 0);
        }
        else  getdisplay().print("---");
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().print("A");

        // Show actual consumption in W
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(260, 260);
        if((powerSensor == "INA219" || powerSensor == "INA226") && simulation == false){
            if(value3 <= 9.9) getdisplay().print(value3, 2);
            if(value3 > 9.9 && value3 <= 99.9)getdisplay().print(value3, 1);
            if(value3 > 99.9) getdisplay().print(value3, 0);
        }
        else  getdisplay().print("---");
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().print("W");

        // Key Layout
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            getdisplay().setCursor(130, 290);
            getdisplay().print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){              // Key for illumination
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
    return new PageSolar(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageSolar(
    "Solar",        // Name of page
    createPage,     // Action
    0,              // Number of bus values depends on selection in Web configuration
    {},             // Names of bus values undepends on selection in Web configuration (refer GwBoatData.h)
    true            // Show display header on/off
);

#endif