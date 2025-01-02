#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"
#include "movingAvg.h"              // Lib for moving average building

class PageVoltage : public Page
{
bool init = false;                  // Marker for init done
bool keylock = false;               // Keylock
uint8_t average = 0;                // Average type [0...3], 0=off, 1=10s, 2=60s, 3=300s
bool trend = true;                  // Trend indicator [0|1], 0=off, 1=on
double raw = 0;
char mode = 'D';                    // display mode (A)nalog | (D)igital

public:
    PageVoltage(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Instantiate PageVoltage");
        if (hasFRAM) {
            average = fram.read(FRAM_VOLTAGE_AVG);
            trend = fram.read(FRAM_VOLTAGE_TREND);
            mode = fram.read(FRAM_VOLTAGE_MODE);
        }
    }
    virtual int handleKey(int key){
         // Change average
        if(key == 1){
            average ++;
            average = average % 4;      // Modulo 4
            if (hasFRAM) fram.write(FRAM_VOLTAGE_AVG, average);
            return 0;                   // Commit the key
        }

        // Switch display mode
        if (key == 2) {
            if (mode == 'A') {
                mode = 'D';
            } else {
                mode = 'A';
            }
            if (hasFRAM) fram.write(FRAM_VOLTAGE_MODE, mode);
            return 0;
        }

        // Trend indicator
        if(key == 5){
            trend = !trend;
            if (hasFRAM) fram.write(FRAM_VOLTAGE_TREND, trend);
            return 0;                   // Commit the key
        }

        // Code for keylock
        if(key == 11){
            keylock = !keylock;         // Toggle keylock
            return 0;                   // Commit the key
        }
        return key;
    }

    void printAvg(int avg, uint16_t x, uint16_t y, bool prefix) {
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(x, y);
        if (prefix) {
            getdisplay().print("Avg: ");
        }
        switch (average) {
            case 0:
                getdisplay().print("1s");
                break;
            case 1:
                getdisplay().print("10s");
                break;
            case 2:
                getdisplay().print("60s");
                break;
            case 3:
                getdisplay().print("300s");
                break;
            default:
                getdisplay().print("1s");
                break;
        }
    }

    void printVoltageSymbol(uint16_t x, uint16_t y, uint16_t color) {
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().setCursor(x, y);
        getdisplay().print("V");
        getdisplay().fillRect(x, y + 6, 22, 3, color);
        getdisplay().fillRect(x, y + 11, 6, 3, color);
        getdisplay().fillRect(x + 8, y + 11, 6, 3, color);
        getdisplay().fillRect(x + 16, y + 11, 6, 3, color);
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData)
    {
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;
        
        // Get config data
        bool simulation = config->getBool(config->useSimuData);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String batVoltage = config->getString(config->batteryVoltage);
        String batType = config->getString(config->batteryType);
        String backlightMode = config->getString(config->backlight);

        double value1 = 0;
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
                break;
            case 1:
                value1 = commonData.data.batteryVoltage10;      // Average 10s
                break;
            case 2:
                value1 = commonData.data.batteryVoltage60;      // Average 60s
                break;
            case 3:
                value1 = commonData.data.batteryVoltage300;     // Average 300s
                break;
            default:
                value1 = commonData.data.batteryVoltage;        // Default
                break;
        }
        bool valid1 = true;

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
        if (raw == 0) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageVoltage, Type:%s %s:=%f", batType, name1.c_str(), raw);

        // Draw page
        //***********************************************************

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        if (mode == 'D') {
            // Display mode digital

            // Show name
            getdisplay().setTextColor(commonData.fgcolor);
            getdisplay().setFont(&Ubuntu_Bold32pt7b);
            getdisplay().setCursor(20, 100);
            getdisplay().print(name1);                           // Value name

            // Show unit
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(270, 100);
            getdisplay().print("V");

            // Show battery type
            getdisplay().setFont(&Ubuntu_Bold8pt7b);
            getdisplay().setCursor(295, 100);
            getdisplay().print(batType);

            // Show average settings
            printAvg(average, 320, 84, true);

            // Reading bus data or using simulation data
            getdisplay().setFont(&DSEG7Classic_BoldItalic60pt7b);
            getdisplay().setCursor(20, 240);
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
                    if(value1 < 10){
                        getdisplay().print(value1,2);
                    }
                    if(value1 >= 10 && value1 < 100){
                        getdisplay().print(value1,1);
                    }
                    if(value1 >= 100){
                        getdisplay().print(value1,0);
                    }
                }
                else{
                getdisplay().print("---");                       // Missing bus data
                }
            }

            // Trend indicator
            // Show trend indicator
            if(trend == true){
                getdisplay().fillRect(310, 240, 40, 120, commonData.bgcolor);   // Clear area
                getdisplay().fillRect(315, 183, 35, 4, commonData.fgcolor);   // Draw separator
                if(int(raw * 10) > int(valueTrend * 10)){
                    displayTrendHigh(320, 174, 11, commonData.fgcolor);  // Show high indicator
                }
                if(int(raw * 10) < int(valueTrend * 10)){
                    displayTrendLow(320, 195, 11, commonData.fgcolor);   // Show low indicator
                }
            }
            // No trend indicator
            else{
                getdisplay().fillRect(310, 240, 40, 120, commonData.bgcolor);   // Clear area
            }

        }
        else {
            // Display mode analog

            // center
            Point c = {260, 270};
            uint8_t r = 240;

            Point p1, p2;
            std::vector<Point> pts;

            // Instrument
            getdisplay().drawCircleHelper(c.x, c.y, r + 2, 0x01, commonData.fgcolor);
            getdisplay().drawCircleHelper(c.x, c.y, r + 1, 0x01, commonData.fgcolor);
            getdisplay().drawCircleHelper(c.x, c.y, r    , 0x01, commonData.fgcolor);

            // Scale
            // angle to voltage scale mapping
            std::map<int, String> mapping = {
                {15, "10"}, {30, "11"}, {45, "12"}, {60, "13"}, {75, "14"}
            };
            pts = {
                {c.x - r, c.y - 1},
                {c.x - r + 12, c.y - 1},
                {c.x - r + 12, c.y + 1},
                {c.x - r, c.y + 1}
            };
            getdisplay().setFont(&Ubuntu_Bold10pt7b);
            for (int angle = 3; angle < 90; angle += 3) {
                if (angle % 15 == 0) {
                    fillPoly4(rotatePoints(c, pts, angle), commonData.fgcolor);
                    p1 = rotatePoint(c, {c.x - r + 30, c.y}, angle);
                    drawTextCenter(p1.x, p1.y, mapping[angle]);
                }
                else {
                    p1 = rotatePoint(c, {c.x - r, c.y}, angle);
                    p2 = rotatePoint(c, {c.x - r + 6, c.y}, angle);
                    getdisplay().drawLine(p1.x, p1.y, p2.x, p2.y, commonData.fgcolor);
                }
            }

            // Pointer rotation and limits
            double angle;
            if (not valid1) {
                angle = -0.5;
            }
            else {
                if (value1 > 15.0) {
                    angle = 91;
                }
                else if (value1 <= 9) {
                    angle = -0.5;
                }
                else {
                    angle = (value1 - 9) * 15;
                }
            }

            // Pointer
            // thick part
            pts = {
                {c.x - 2, c.y + 3},
                {c.x - r + 38, c.y + 2},
                {c.x - r + 38, c.y - 2},
                {c.x - 2, c.y - 3}
            };
            fillPoly4(rotatePoints(c, pts, angle), commonData.fgcolor);
            // thin part
            pts = {
                {c.x - r + 40, c.y + 1},
                {c.x - r + 5, c.y + 1},
                {c.x - r + 5, c.y -1},
                {c.x - r + 40, c.y - 1},
            };
            fillPoly4(rotatePoints(c, pts, angle), commonData.fgcolor);

            // base
            getdisplay().fillCircle(c.x, c.y, 7, commonData.fgcolor);
            getdisplay().fillCircle(c.x, c.y, 4, commonData.bgcolor);

            // Symbol
            printVoltageSymbol(40, 60, commonData.fgcolor);

            // Additional information at right side
            getdisplay().setFont(&Ubuntu_Bold8pt7b);
            getdisplay().setCursor(300, 60);
            getdisplay().print("Source:");
            getdisplay().setCursor(300, 80);
            getdisplay().print(name1);

            getdisplay().setCursor(300, 110);
            getdisplay().print("Type:");
            getdisplay().setCursor(300, 130);
            getdisplay().print(batType);

            getdisplay().setCursor(300, 160);
            getdisplay().print("Avg:");
            printAvg(average, 300, 180, false);

            // FRAM indicator
            if (hasFRAM) {
                getdisplay().drawXBitmap(300, 240, fram_bits, fram_width, fram_height, commonData.fgcolor);
            }

        }

        // Key Layout
        getdisplay().setTextColor(commonData.fgcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            getdisplay().setCursor(10, 290);
            getdisplay().print("[AVG]");
            getdisplay().setCursor(62, 290);
            getdisplay().print("[MODE]");
            getdisplay().setCursor(130, 290);
            getdisplay().print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            getdisplay().setCursor(293, 290);
                getdisplay().print("[TRD]");
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
    return new PageVoltage(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageVoltage(
    "Voltage",      // Name of page
    createPage,     // Action
    0,              // Number of bus values depends on selection in Web configuration
    {},             // Names of bus values undepends on selection in Web configuration (refer GwBoatData.h)
    true            // Show display header on/off
);

#endif
