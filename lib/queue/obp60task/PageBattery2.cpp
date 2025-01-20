#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

#include "Pagedata.h"
#include "OBP60Extensions.h"
#include "movingAvg.h"              // Lib for moving average building

class PageBattery2 : public Page
{
bool init = false;                  // Marker for init done
bool keylock = false;               // Keylock
int average = 0;                    // Average type [0...3], 0=off, 1=10s, 2=60s, 3=300s
bool trend = true;                  // Trend indicator [0|1], 0=off, 1=on
double raw = 0;

public:
    PageBattery2(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageBattery2");
    }
    virtual int handleKey(int key){
         // Change average
        if(key == 1){
            average ++;
            average = average % 4;      // Modulo 4
            return 0;                   // Commit the key
        }

        // Trend indicator
        if(key == 5){
            trend = !trend;
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

        // Polynominal coefficients second order for battery energy level calculation
        // index 0 = Pb, 1 = Gel, 2 = AGM, 3 = LiFePo4
        float x0[4] = {+3082.5178, +1656.1571, +1316.8766, +14986.9336};    // Offset
        float x1[4] = {-603.7478, -351.6503, -298.1454, -2432.1985};        // X
        float x2[4] = {+29.0340, +17.9000, +15.8196, +98.6132};             // X²
        int batPercentage = 0;      // Battery level
        float batRange = 0;         // Range in hours
        
        // Get config data
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String batVoltage = config->getString(config->batteryVoltage);
        int batCapacity = config->getInt(config->batteryCapacity);
        String batType = config->getString(config->batteryType);
        String backlightMode = config->getString(config->backlight);
        String powerSensor = config->getString(config->usePowSensor1);

        double value1 = 0;  // Battery voltage
        double value2 = 0;  // Battery current
        double value3 = 0;  // Battery power consumption
        double valueTrend = 0;  // Average over 10 values
        
        // Get voltage value
        String name1 = "VBat";

        // Create trend value
        if(init == false){          // Load start values for first page run
            valueTrend = commonData.data.batteryVoltage10;
            init = true;
        }
        else{                       // Reading trend value
            valueTrend = commonData.data.batteryVoltage10;
        }

        // Get raw value for trend indicator
        raw = commonData.data.batteryVoltage;        // Live data

        // Switch average values
        switch (average) {
            case 0:
                value1 = commonData.data.batteryVoltage;        // Live data
                value2 = commonData.data.batteryCurrent;
                value3 = commonData.data.batteryPower;
                break;
            case 1:
                value1 = commonData.data.batteryVoltage10;      // Average 10s
                value2 = commonData.data.batteryCurrent10;
                value3 = commonData.data.batteryPower10;
                break;
            case 2:
                value1 = commonData.data.batteryVoltage60;      // Average 60s
                value2 = commonData.data.batteryCurrent60;
                value3 = commonData.data.batteryPower60;
                break;
            case 3:
                value1 = commonData.data.batteryVoltage300;     // Average 300s
                value2 = commonData.data.batteryCurrent300;
                value3 = commonData.data.batteryPower300;
                break;
            default:
                value1 = commonData.data.batteryVoltage;        // Default
                value2 = commonData.data.batteryCurrent;
                value3 = commonData.data.batteryPower;
                break;
        }
        bool valid1 = true;

        // Battery energy level calculation
        if(String(batType) == "Pb"){
            batPercentage = (value1 * value1 * x2[0]) + (value1 * x1[0]) + x0[0];
        }
        if(String(batType) == "Gel"){
            batPercentage = (value1 * value1 * x2[1]) + (value1 * x1[1]) + x0[1];
        }
        if(String(batType) == "AGM"){
            batPercentage = (value1 * value1 * x2[2]) + (value1 * x1[2]) + x0[2];
        }
        if(String(batType) == "LiFePo4"){
            batPercentage = (value1 * value1 * x2[3]) + (value1 * x1[3]) + x0[3];
        }
        // Limits for battery level
        if(batPercentage < 0) batPercentage = 0;
        if(batPercentage > 99) batPercentage = 99;

        // Battery range calculation
        if(value2 <= 0) value2 = 0.0000001; // Limiting current
        batRange = batCapacity * batPercentage / 100 / value2;
        // Limits for battery range
        if(batRange < 0) batRange = 0;
        if(batRange > 99) batRange = 99;

        // Optical warning by limit violation
        if(String(flashLED) == "Limit Violation"){
            // Limits for Pb battery
            if(String(batType) == "Pb" && (raw < 11.8 || raw > 14.8)){
                setBlinkingLED(true);
            }
            if(String(batType) == "Pb" && (raw >= 11.8 && raw <= 14.8)){
                setBlinkingLED(false);
                setFlashLED(false);
            }
            // Limits for Gel battery
            if(String(batType) == "Gel" && (raw < 11.8 || raw > 14.4)){
                setBlinkingLED(true);
            }
            if(String(batType) == "Gel" && (raw >= 11.8 && raw <= 14.4)){
                setBlinkingLED(false);
                setFlashLED(false);
            }
            // Limits for AGM battery
            if(String(batType) == "AGM" && (raw < 11.8 || raw > 14.7)){
                setBlinkingLED(true);
            }
            if(String(batType) == "AGM" && (raw >= 11.8 && raw <= 14.7)){
                setBlinkingLED(false);
                setFlashLED(false);
            }
            // Limits for LiFePo4 battery
            if(String(batType) == "LiFePo4" && (raw < 12.0 || raw > 14.6)){
                setBlinkingLED(true);
            }
            if(String(batType) == "LiFePo4" && (raw >= 12.0 && raw <= 14.6)){
                setBlinkingLED(false);
                setFlashLED(false);
            }
        }
        
        // Logging voltage value
        LOG_DEBUG(GwLog::LOG,"Drawing at PageBattery2, Type:%s %s:=%f", batType.c_str(), name1.c_str(), raw);

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

        // Show batery capacity
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(10, 200);
        if(batCapacity <= 999) getdisplay().print(batCapacity, 0);
        if(batCapacity > 999) getdisplay().print(float(batCapacity/1000.0), 1);
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        if(batCapacity <= 999) getdisplay().print("Ah");
        if(batCapacity > 999) getdisplay().print("kAh");

        // Show info
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(10, 235);
        getdisplay().print("Installed");
        getdisplay().setCursor(10, 255);
        getdisplay().print("Battery Type");

        // Show battery with fill level
        batteryGraphic(150, 45, batPercentage, pixelcolor, bgcolor);

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
        getdisplay().print(batPercentage);
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().print("%");

        // Show time to full discharge
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(150, 260);
        if((powerSensor == "INA219" || powerSensor == "INA226") && simulation == false){
            if(batRange < 9.9) getdisplay().print(batRange, 1);
            else getdisplay().print(batRange, 0);
        }
        else  getdisplay().print("--");
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().print("h");

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
    return new PageBattery2(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageBattery2(
    "Battery2",     // Name of page
    createPage,     // Action
    0,              // Number of bus values depends on selection in Web configuration
    {},             // Names of bus values undepends on selection in Web configuration (refer GwBoatData.h)
    true            // Show display header on/off
);

#endif