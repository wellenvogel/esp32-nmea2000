#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"
#include "movingAvg.h"              // Lib for moving average building

class PageGenerator : public Page
{
bool init = false;                  // Marker for init done
bool keylock = false;               // Keylock
int average = 0;                    // Average type [0...3], 0=off, 1=10s, 2=60s, 3=300s

public:
    PageGenerator(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageGenerator");
    }
    virtual int handleKey(int key){
         // Change average
        if(key == 1){
            average ++;
            average = average % 4;      // Modulo 4
            return 0;                   // Commit the key
        }

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
        int genPower = config->getInt(config->genPower);
        String batType = config->getString(config->batteryType);
        String backlightMode = config->getString(config->backlight);
        String powerSensor = config->getString(config->usePowSensor3);

        double value1 = 0;  // Battery voltage
        double value2 = 0;  // Battery current
        double value3 = 0;  // Battery power consumption
        double valueTrend = 0;  // Average over 10 values
        int genPercentage = 0;
        
        // Get voltage value
        String name1 = "VGen";

        // Read values
        value1 = commonData.data.batteryVoltage;        // Live data
        value2 = commonData.data.batteryCurrent;
        value3 = commonData.data.batteryPower;
        genPercentage = value3 * 100 / (double)genPower;    // Load value
        bool valid1 = true;

        // Limits for battery level
        if(genPercentage < 0) genPercentage = 0;
        if(genPercentage > 99) genPercentage = 99;

        // Optical warning by limit violation
        if(String(flashLED) == "Limit Violation"){
            // Over voltage
            if(value1 > 14.8 && batVoltage == "12V"){
                setBlinkingLED(true);
            }
            if(value1 <= 14.8 && batVoltage == "12V"){
                setBlinkingLED(false);
            }
            if(value1 > 29.6 && batVoltage == "24V"){
                setBlinkingLED(true);
            }
            if(value1 <= 29.6 && batVoltage == "24V"){
                setBlinkingLED(false);
            }     
        }
        
        // Logging voltage value
        if (value1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageGenerator, Type:%s %s:=%f", batType, name1, value1);

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

        // Show name
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(10, 65);
        getdisplay().print("Bat.");

         // Show batery type
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(90, 65);
        getdisplay().print(batType);

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

        // Show solar power
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(10, 200);
        if(genPower <= 999) getdisplay().print(genPower, 0);
        if(genPower > 999) getdisplay().print(float(genPower/1000.0), 1);
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        if(genPower <= 999) getdisplay().print("w");
        if(genPower > 999) getdisplay().print("kW");

        // Show info
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(10, 235);
        getdisplay().print("Installed");
        getdisplay().setCursor(10, 255);
        getdisplay().print("Battery Type");

        // Show battery with fill level
        batteryGraphic(150, 45, genPercentage, pixelcolor, bgcolor);

        // Show average settings
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(150, 145);
        switch (average) {
            case 0:
                getdisplay().print("Avg: 1s");
                break;
            case 1:
                getdisplay().print("Avg: 10s");
                break;
            case 2:
                getdisplay().print("Avg: 60s");
                break;
            case 3:
                getdisplay().print("Avg: 300s");
                break;
            default:
                getdisplay().print("Avg: 1s");
                break;
        } 

        // Show fill level in percent
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(150, 200);
        getdisplay().print(genPercentage);
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
            i2cAddr = " (0x" + String(INA226_I2C_ADDR1, HEX) + ")";
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
            getdisplay().setCursor(10, 290);
            getdisplay().print("[AVG]");
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