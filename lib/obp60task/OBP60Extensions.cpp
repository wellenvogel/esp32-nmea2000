#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

#include <Arduino.h>
#include <PCF8574.h>      // Driver for PCF8574 output modul from Horter
#include <Wire.h>         // I2C
#include <RTClib.h>       // Driver for DS1388 RTC
#include "SunRise.h"      // Lib for sunrise and sunset calculation
#include "Pagedata.h"
#include "OBP60Hardware.h"
#include "OBP60Extensions.h"
#include "imglib.h"

// Character sets
#include "Ubuntu_Bold8pt7b.h"
#include "Ubuntu_Bold10pt7b.h"
#include "Ubuntu_Bold12pt7b.h"
#include "Ubuntu_Bold16pt7b.h"
#include "Ubuntu_Bold20pt7b.h"
#include "Ubuntu_Bold32pt7b.h"
#include "DSEG7Classic-BoldItalic16pt7b.h"
#include "DSEG7Classic-BoldItalic20pt7b.h"
#include "DSEG7Classic-BoldItalic30pt7b.h"
#include "DSEG7Classic-BoldItalic42pt7b.h"
#include "DSEG7Classic-BoldItalic60pt7b.h"
#include "Atari16px8b.h" // Key label font

// E-Ink Display
#define GxEPD_WIDTH 400     // Display width
#define GxEPD_HEIGHT 300    // Display height

#ifdef DISPLAY_GDEW042T2
// Set display type and SPI pins for display
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(OBP_SPI_CS, OBP_SPI_DC, OBP_SPI_RST, OBP_SPI_BUSY)); // GDEW042T2 400x300, UC8176 (IL0398)
// Export display in new funktion
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> & getdisplay(){return display;}
#endif

#ifdef DISPLAY_GDEY042T81
// Set display type and SPI pins for display
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(OBP_SPI_CS, OBP_SPI_DC, OBP_SPI_RST, OBP_SPI_BUSY)); // GDEW042T2 400x300, UC8176 (IL0398)
// Export display in new funktion
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> & getdisplay(){return display;}
#endif

#ifdef DISPLAY_GYE042A87
// Set display type and SPI pins for display
GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> display(GxEPD2_420_GYE042A87(OBP_SPI_CS, OBP_SPI_DC, OBP_SPI_RST, OBP_SPI_BUSY)); // GDEW042T2 400x300, UC8176 (IL0398)
// Export display in new funktion
GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> & getdisplay(){return display;}
#endif

#ifdef DISPLAY_SE0420NQ04
// Set display type and SPI pins for display
GxEPD2_BW<GxEPD2_420_SE0420NQ04, GxEPD2_420_SE0420NQ04::HEIGHT> display(GxEPD2_420_SE0420NQ04(OBP_SPI_CS, OBP_SPI_DC, OBP_SPI_RST, OBP_SPI_BUSY)); // GDEW042T2 400x300, UC8176 (IL0398)
// Export display in new funktion
GxEPD2_BW<GxEPD2_420_SE0420NQ04, GxEPD2_420_SE0420NQ04::HEIGHT> & getdisplay(){return display;}
#endif

// Horter I2C moduls
PCF8574 pcf8574_Out(PCF8574_I2C_ADDR1); // First digital output modul PCF8574 from Horter

// FRAM
Adafruit_FRAM_I2C fram;
bool hasFRAM = false;

// Global vars
bool blinkingLED = false;       // Enable / disable blinking flash LED
bool statusLED = false;         // Actual status of flash LED on/off
bool statusBacklightLED = false;// Actual status of flash LED on/off

int uvDuration = 0;             // Under voltage duration in n x 100ms

RTC_DATA_ATTR uint8_t RTC_lastpage; // Remember last page while deep sleeping


LedTaskData *ledTaskData=nullptr;

void hardwareInit(GwApi *api)
{
    Wire.begin();
    // Init PCF8574 digital outputs
    Wire.setClock(I2C_SPEED);       // Set I2C clock on 10 kHz
    if(pcf8574_Out.begin()){        // Initialize PCF8574
        pcf8574_Out.write8(255);    // Clear all outputs
    }
    fram = Adafruit_FRAM_I2C();
    if (esp_reset_reason() ==  ESP_RST_POWERON) {
        // help initialize FRAM
        api->getLogger()->logDebug(GwLog::LOG,"Delaying I2C init for 250ms due to cold boot");
        delay(250);
    }
    // FRAM (e.g. MB85RC256V)
    if (fram.begin(FRAM_I2C_ADDR)) {
        hasFRAM = true;
        uint16_t manufacturerID;
        uint16_t productID;
        fram.getDeviceID(&manufacturerID, &productID);
        // Boot counter
        uint8_t framcounter = fram.read(0x0000);
        fram.write(0x0000, framcounter+1);
        api->getLogger()->logDebug(GwLog::LOG,"FRAM detected: 0x%04x/0x%04x (counter=%d)", manufacturerID, productID, framcounter);
    }
    else {
        hasFRAM = false;
        api->getLogger()->logDebug(GwLog::LOG,"NO FRAM detected");
    }
}

void setPortPin(uint pin, bool value){
    pinMode(pin, OUTPUT);
    digitalWrite(pin, value);
}

void togglePortPin(uint pin){
    pinMode(pin, OUTPUT);
    digitalWrite(pin, !digitalRead(pin));
}

void startLedTask(GwApi *api){
    ledTaskData=new LedTaskData(api);
    createSpiLedTask(ledTaskData);
}

uint8_t getLastPage() {
    return RTC_lastpage;
}

#ifdef BOARD_OBP60S3
void deepSleep(CommonData &common){
    RTC_lastpage = common.data.actpage - 1;
    // Switch off all power lines
    setPortPin(OBP_BACKLIGHT_LED, false);   // Backlight Off
    setFlashLED(false);                     // Flash LED Off
    buzzer(TONE4, 20);                      // Buzzer tone 4kHz 20ms
    // Shutdown EInk display
    getdisplay().setFullWindow();               // Set full Refresh
    getdisplay().fillScreen(common.bgcolor);    // Clear screen
    getdisplay().setTextColor(common.fgcolor);
    getdisplay().setFont(&Ubuntu_Bold20pt7b);
    getdisplay().setCursor(85, 150);
    getdisplay().print("Sleep Mode");
    getdisplay().setFont(&Ubuntu_Bold8pt7b);
    getdisplay().setCursor(65, 175);
    getdisplay().print("To wake up press key and wait 5s");
    getdisplay().nextPage();                // Update display contents
    getdisplay().powerOff();                // Display power off
    setPortPin(OBP_POWER_50, false);        // Power off ePaper display
    // Stop system
    esp_deep_sleep_start();                 // Deep Sleep with weakup via touch pin
}
#endif
#ifdef BOARD_OBP40S3
// Deep sleep funktion
void deepSleep(CommonData &common){
    RTC_lastpage = common.data.actpage - 1;
    // Switch off all power lines
    setPortPin(OBP_BACKLIGHT_LED, false);   // Backlight Off
    setFlashLED(false);                     // Flash LED Off
    // Shutdown EInk display
    getdisplay().setFullWindow();               // Set full Refresh
    //getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update
    getdisplay().fillScreen(common.bgcolor);    // Clear screen
    getdisplay().setTextColor(common.fgcolor);
    getdisplay().setFont(&Ubuntu_Bold20pt7b);
    getdisplay().setCursor(85, 150);
    getdisplay().print("Sleep Mode");
    getdisplay().setFont(&Ubuntu_Bold8pt7b);
    getdisplay().setCursor(65, 175);
    getdisplay().print("To wake up press wheel and wait 5s");
    getdisplay().nextPage();                // Partial update
    getdisplay().powerOff();                // Display power off
    setPortPin(OBP_POWER_EPD, false);       // Power off ePaper display
    setPortPin(OBP_POWER_SD, false);        // Power off SD card
    // Stop system
    esp_deep_sleep_start();             // Deep Sleep with weakup via GPIO pin
}
#endif

// Valid colors see hue
Color colorMapping(const String &colorString){
    Color color = COLOR_RED;
    if(colorString == "Orange"){color = Color(255,153,0);}
    if(colorString == "Yellow"){color = Color(255,255,0);}
    if(colorString == "Green"){color = COLOR_GREEN;}
    if(colorString == "Blue"){color = COLOR_BLUE;}
    if(colorString == "Aqua"){color = Color(51,102,255);}
    if(colorString == "Violet"){color = Color(255,0,102);}
    if(colorString == "White"){color = COLOR_WHITE;}
    return color;
}

BacklightMode backlightMapping(const String &backlightString) {
    static std::map<String, BacklightMode> const table = {
        {"Off", BacklightMode::OFF},
        {"Control by Bus", BacklightMode::BUS},
        {"Control by Time", BacklightMode::TIME},
        {"Control by Key", BacklightMode::KEY},
        {"On", BacklightMode::ON},
    };
    auto it = table.find(backlightString);
    if (it != table.end()) {
        return it->second;
    }
    return BacklightMode::OFF;
}

// All defined colors see pixeltypes.h in FastLED lib
void setBacklightLED(uint brightness, const Color &color){
    if (ledTaskData == nullptr) return;
    Color nv=setBrightness(color,brightness);
    LedInterface current=ledTaskData->getLedData();
    current.setBacklight(nv);
    ledTaskData->setLedData(current);    
}

void toggleBacklightLED(uint brightness, const Color &color){
    if (ledTaskData == nullptr) return;
    statusBacklightLED = !statusBacklightLED;
    Color nv=setBrightness(statusBacklightLED?color:COLOR_BLACK,brightness);
    LedInterface current=ledTaskData->getLedData();
    current.setBacklight(nv);
    ledTaskData->setLedData(current); 
}

void setFlashLED(bool status){
    if (ledTaskData == nullptr) return;
    Color c=status?COLOR_RED:COLOR_BLACK;
    LedInterface current=ledTaskData->getLedData();
    current.setFlash(c);
    ledTaskData->setLedData(current);
}

void blinkingFlashLED(){
    if(blinkingLED == true){
        statusLED = !statusLED;     // Toggle LED for each run
        setFlashLED(statusLED);
    }    
}

void setBlinkingLED(bool status){
    blinkingLED = status;
}

uint buzzerpower = 50;

void buzzer(uint frequency, uint duration){
    if(frequency > 8000){   // Max 8000Hz
        frequency = 8000;
    }
    if(buzzerpower > 100){  // Max 100%
        buzzerpower = 100;
    }
    if(duration > 1000){    // Max 1000ms
        duration = 1000;
    }
    
    // Using LED PWM function for sound generation
    pinMode(OBP_BUZZER, OUTPUT);
    ledcSetup(0, frequency, 8);         // Ch 0, ferquency in Hz, 8 Bit resolution of PWM
    ledcAttachPin(OBP_BUZZER, 0);
    ledcWrite(0, uint(buzzerpower * 1.28));    // 50% duty cycle are 100%
    delay(duration);
    ledcWrite(0, 0);                    // 0% duty cycle are 0%
}

void setBuzzerPower(uint power){
    buzzerpower = power;
}

// Delete xdr prefix from string
String xdrDelete(String input){
    if(input.substring(0,3) == "xdr"){
        input = input.substring(3, input.length());
    }
    return input;
}

Point rotatePoint(const Point& origin, const Point& p, double angle) {
    // rotate poind around origin by degrees
    Point rotated;
    double phi = angle * M_PI / 180.0;
    double dx = p.x - origin.x;
    double dy = p.y - origin.y;
    rotated.x = origin.x + cos(phi) * dx - sin(phi) * dy;
    rotated.y = origin.y + sin(phi) * dx + cos(phi) * dy;
    return rotated;
}

std::vector<Point> rotatePoints(const Point& origin, const std::vector<Point>& pts, double angle) {
    std::vector<Point> rotatedPoints;
    for (const auto& p : pts) {
         rotatedPoints.push_back(rotatePoint(origin, p, angle));
    }
     return rotatedPoints;
}

void fillPoly4(const std::vector<Point>& p4, uint16_t color) {
    getdisplay().fillTriangle(p4[0].x, p4[0].y, p4[1].x, p4[1].y, p4[2].x, p4[2].y, color);
    getdisplay().fillTriangle(p4[0].x, p4[0].y, p4[2].x, p4[2].y, p4[3].x, p4[3].y, color);
}

// Draw centered text
void drawTextCenter(int16_t cx, int16_t cy, String text) {
    int16_t x1, y1;
    uint16_t w, h;
    getdisplay().getTextBounds(text, 0, 150, &x1, &y1, &w, &h);
    getdisplay().setCursor(cx - w / 2, cy + h / 2);
    getdisplay().print(text);
}

// Draw right aligned text
void drawTextRalign(int16_t x, int16_t y, String text) {
    int16_t x1, y1;
    uint16_t w, h;
    getdisplay().getTextBounds(text, 0, 150, &x1, &y1, &w, &h);
    getdisplay().setCursor(x - w, y);
    getdisplay().print(text);
}

// Show a triangle for trend direction high (x, y is the left edge)
void displayTrendHigh(int16_t x, int16_t y, uint16_t size, uint16_t color){
    getdisplay().fillTriangle(x, y, x+size*2, y, x+size, y-size*2, color);
}

// Show a triangle for trend direction low (x, y is the left edge)
void displayTrendLow(int16_t x, int16_t y, uint16_t size, uint16_t color){
    getdisplay().fillTriangle(x, y, x+size*2, y, x+size, y+size*2, color);
}

// Show header informations
void displayHeader(CommonData &commonData, GwApi::BoatValue *date, GwApi::BoatValue *time, GwApi::BoatValue *hdop){

    static bool heartbeat = false;
    static unsigned long usbRxOld = 0;
    static unsigned long usbTxOld = 0;
    static unsigned long serRxOld = 0;
    static unsigned long serTxOld = 0;
    static unsigned long tcpSerRxOld = 0;
    static unsigned long tcpSerTxOld = 0;
    static unsigned long tcpClRxOld = 0;
    static unsigned long tcpClTxOld = 0;
    static unsigned long n2kRxOld = 0;
    static unsigned long n2kTxOld = 0;

    if(commonData.config->getBool(commonData.config->statusLine) == true){

        // Show status info
        getdisplay().setTextColor(commonData.fgcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(0, 15);
        if(commonData.status.wifiApOn){
        getdisplay().print(" AP ");
        }
        // If receive new telegram data then display bus name
        if(commonData.status.tcpClRx != tcpClRxOld || commonData.status.tcpClTx != tcpClTxOld || commonData.status.tcpSerRx != tcpSerRxOld || commonData.status.tcpSerTx != tcpSerTxOld){
        getdisplay().print("TCP ");
        }
        if(commonData.status.n2kRx != n2kRxOld || commonData.status.n2kTx != n2kTxOld){
        getdisplay().print("N2K ");
        }
        if(commonData.status.serRx != serRxOld || commonData.status.serTx != serTxOld){
        getdisplay().print("183 ");
        }
        if(commonData.status.usbRx != usbRxOld || commonData.status.usbTx != usbTxOld){
        getdisplay().print("USB ");
        }
        double gpshdop = formatValue(hdop, commonData).value;
        if(commonData.config->getString(commonData.config->useGPS) != "off" &&  gpshdop <= commonData.config->getInt(commonData.config->hdopAccuracy) && hdop->valid == true){
        getdisplay().print("GPS");
        }
        // Save old telegram counter
        tcpClRxOld = commonData.status.tcpClRx;
        tcpClTxOld = commonData.status.tcpClTx;
        tcpSerRxOld = commonData.status.tcpSerRx;
        tcpSerTxOld = commonData.status.tcpSerTx;
        n2kRxOld = commonData.status.n2kRx;
        n2kTxOld = commonData.status.n2kTx;
        serRxOld = commonData.status.serRx;
        serTxOld = commonData.status.serTx;
        usbRxOld = commonData.status.usbRx;
        usbTxOld = commonData.status.usbTx;

#ifdef HARDWARE_V21
        // Display key lock status
        if (commonData.keylock) {
            getdisplay().drawXBitmap(170, 1, lock_bits, icon_width, icon_height, commonData.fgcolor);
        } else {
            getdisplay().drawXBitmap(166, 1, swipe_bits, swipe_width, swipe_height, commonData.fgcolor);
        }
#endif
#ifdef LIPO_ACCU_1200
        if (commonData.data.BatteryChargeStatus == 1) {
             getdisplay().drawXBitmap(170, 1, battery_loading_bits, battery_width, battery_height, commonData.fgcolor);
        } else {
#ifdef VOLTAGE_SENSOR
            if (commonData.data.batteryLevelLiPo < 10) {
                getdisplay().drawXBitmap(170, 1, battery_0_bits, battery_width, battery_height, commonData.fgcolor);
            } else if (commonData.data.batteryLevelLiPo < 25) {
                getdisplay().drawXBitmap(170, 1, battery_25_bits, battery_width, battery_height, commonData.fgcolor);
            } else if (commonData.data.batteryLevelLiPo < 50) {
                getdisplay().drawXBitmap(170, 1, battery_50_bits, battery_width, battery_height, commonData.fgcolor);
            } else if (commonData.data.batteryLevelLiPo < 75) {
                getdisplay().drawXBitmap(170, 1, battery_75_bits, battery_width, battery_height, commonData.fgcolor);
            } else {
                getdisplay().drawXBitmap(170, 1, battery_100_bits, battery_width, battery_height, commonData.fgcolor);
            }
#endif // VOLTAGE_SENSOR
        }
#endif // LIPO_ACCU_1200

        // Heartbeat as page number
        if (heartbeat) {
            getdisplay().setTextColor(commonData.bgcolor);
            getdisplay().fillRect(201, 0, 23, 19, commonData.fgcolor);
        } else {
            getdisplay().setTextColor(commonData.fgcolor);
            getdisplay().drawRect(201, 0, 23, 19, commonData.fgcolor);
        }
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        drawTextCenter(211, 9, String(commonData.data.actpage));
        heartbeat = !heartbeat;

        // Date and time
        String fmttype = commonData.config->getString(commonData.config->dateFormat);
        String timesource = commonData.config->getString(commonData.config->timeSource);
        double tz = commonData.config->getString(commonData.config->timeZone).toDouble();
        getdisplay().setTextColor(commonData.fgcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(230, 15);
        if (timesource == "RTC" or timesource == "iRTC") {
            // TODO take DST into account
            if (commonData.data.rtcValid) {
                time_t tv = mktime(&commonData.data.rtcTime) + (int)(tz * 3600);
                struct tm *local_tm = localtime(&tv);
                getdisplay().print(formatTime('m', local_tm->tm_hour, local_tm->tm_min, 0));
                getdisplay().print(" ");
                getdisplay().print(formatDate(fmttype, local_tm->tm_year + 1900, local_tm->tm_mon + 1, local_tm->tm_mday));
                getdisplay().print(" ");
                getdisplay().print(tz == 0 ? "UTC" : "LOT");
            } else {
                drawTextRalign(396, 15, "RTC invalid");
            }
        }
        else if (timesource == "GPS") {
            // Show date and time if date present
            if(date->valid == true){
                String acttime = formatValue(time, commonData).svalue;
                acttime = acttime.substring(0, 5);
                String actdate = formatValue(date, commonData).svalue;
                getdisplay().print(acttime);
                getdisplay().print(" ");
                getdisplay().print(actdate);
                getdisplay().print(" ");
                getdisplay().print(tz == 0 ? "UTC" : "LOT");
            }
            else{
                if(commonData.config->getBool(commonData.config->useSimuData) == true){
                    getdisplay().print("12:00 01.01.2024 LOT");
                }
                else{
                    drawTextRalign(396, 15, "No GPS data");
                }
            }
        } // timesource == "GPS"
        else {
            getdisplay().print("No time source");
        }
    }
}

void displayFooter(CommonData &commonData) {

    getdisplay().setFont(&Atari16px);
    getdisplay().setTextColor(commonData.fgcolor);

#ifdef HARDWARE_V21
    // Frame around key icon area
    if (! commonData.keylock) {
        // horizontal elements
        const uint16_t top = 280;
        const uint16_t bottom = 299;
        // horizontal stub lines
        getdisplay().drawLine(commonData.keydata[0].x, top, commonData.keydata[0].x+10, top, commonData.fgcolor);
        for (int i = 1; i <= 5; i++) {
            getdisplay().drawLine(commonData.keydata[i].x-10, top, commonData.keydata[i].x+10, top, commonData.fgcolor);
        }
        getdisplay().drawLine(commonData.keydata[5].x + commonData.keydata[5].w - 10, top, commonData.keydata[5].x + commonData.keydata[5].w + 1, top, commonData.fgcolor);
        // vertical key separators
        for (int i = 0; i <= 4; i++) {
            getdisplay().drawLine(commonData.keydata[i].x + commonData.keydata[i].w, top, commonData.keydata[i].x + commonData.keydata[i].w, bottom, commonData.fgcolor);
        }
        for (int i = 0; i < 6; i++) {
            uint16_t x, y;
            if (commonData.keydata[i].label.length() > 0) {
                // check if icon is enabled
                String icon_name = commonData.keydata[i].label.substring(1);
                if (commonData.keydata[i].label[0] == '#') {
                    if (iconmap.find(icon_name) != iconmap.end()) {
                        x = commonData.keydata[i].x + (commonData.keydata[i].w - icon_width) / 2;
                        y = commonData.keydata[i].y + (commonData.keydata[i].h - icon_height) / 2;
                        getdisplay().drawXBitmap(x, y, iconmap[icon_name], icon_width, icon_height, commonData.fgcolor);
                    } else {
                        // icon is missing, use name instead
                        x = commonData.keydata[i].x + commonData.keydata[i].w / 2;
                        y = commonData.keydata[i].y + commonData.keydata[i].h / 2;
                        drawTextCenter(x, y, icon_name);
                    }
                } else {
                    x = commonData.keydata[i].x + commonData.keydata[i].w / 2;
                    y = commonData.keydata[i].y + commonData.keydata[i].h / 2;
                    drawTextCenter(x, y, commonData.keydata[i].label);
                }
            }
        }
    } else {
        getdisplay().setCursor(65, 295);
        getdisplay().print("Press 1 and 6 fast to unlock keys");
    }
#endif
#ifdef BOARD_OBP40S3
    // grapical page indicator
    static const uint16_t r = 5;
    static const uint16_t space = 4;
    uint16_t w = commonData.data.maxpage * r * 2 + (commonData.data.maxpage - 1) * space;
    uint16_t x0 = (GxEPD_WIDTH - w) / 2 + r * 2;
    for (int i = 0; i < commonData.data.maxpage; i++) {
        if (i == (commonData.data.actpage - 1)) {
            getdisplay().fillCircle(x0 + i * (r * 2 + space), 290, r, commonData.fgcolor);
        } else {
            getdisplay().drawCircle(x0 + i * (r * 2 + space), 290, r, commonData.fgcolor);
        }
    }
    // key indicators
    // left side = top key "menu"
    getdisplay().drawLine(0, 280, 10, 280, commonData.fgcolor);
    getdisplay().drawLine(55, 280, 65, 280, commonData.fgcolor);
    getdisplay().drawLine(65, 280, 65, 299, commonData.fgcolor);
    drawTextCenter(33, 291, commonData.keydata[0].label);
    // right side = bottom key "exit"
    getdisplay().drawLine(390, 280, 399, 280, commonData.fgcolor);
    getdisplay().drawLine(335, 280, 345, 280, commonData.fgcolor);
    getdisplay().drawLine(335, 280, 335, 399, commonData.fgcolor);
    drawTextCenter(366, 291, commonData.keydata[1].label);
#endif
}

// Sunset und sunrise calculation
SunData calcSunsetSunrise(GwApi *api, double time, double date, double latitude, double longitude, double timezone){
    GwLog *logger=api->getLogger();
    SunData returnset;
    SunRise sr;
    int secPerHour = 3600;
    int secPerYear = 86400;
    sr.hasRise = false;
    sr.hasSet = false;
    time_t t = 0;
    time_t sunR = 0;
    time_t sunS = 0;

    if (!isnan(time) && !isnan(date) && !isnan(latitude) && !isnan(longitude) && !isnan(timezone)) {
        
        // Calculate local epoch
        t = (date * secPerYear) + time;    
        // api->getLogger()->logDebug(GwLog::DEBUG,"... calcSun: Lat %f, Lon  %f, at: %d ", latitude, longitude, t);
        sr.calculate(latitude, longitude, t);       // LAT, LON, EPOCH
        // Sunrise
        if (sr.hasRise) {
            sunR = (sr.riseTime + int(timezone * secPerHour) + 30) % secPerYear; // add 30 seconds: round to minutes
            returnset.sunriseHour = int (sunR / secPerHour);
            returnset.sunriseMinute = int((sunR - returnset.sunriseHour * secPerHour)/60);
        }
        // Sunset
        if (sr.hasSet)  {
            sunS = (sr.setTime  + int(timezone * secPerHour) + 30) % secPerYear; // add 30 seconds: round to minutes
            returnset.sunsetHour = int (sunS / secPerHour);
            returnset.sunsetMinute = int((sunS - returnset.sunsetHour * secPerHour)/60);      
        }
        // Sun control (return value by sun on sky = false, sun down = true)
        if ((t >= sr.riseTime) && (t <= sr.setTime))      
             returnset.sunDown = false; 
        else returnset.sunDown = true;
    }
    // Return values
    return returnset;
}

// Battery graphic with fill level
void batteryGraphic(uint x, uint y, float percent, int pcolor, int bcolor){
    // Show battery
        int xb = x;     // X position
        int yb = y;     // Y position
        int t = 4;      // Line thickness
        // Percent limits
        if(percent < 0){
            percent = 0;
        }
         if(percent > 99){
            percent = 99;
        }
        // Battery corpus 100x80 with fill level
        int level = int((100.0 - percent) * (80-(2*t)) / 100.0);
        getdisplay().fillRect(xb, yb, 100, 80, pcolor);
        if(percent < 99){
            getdisplay().fillRect(xb+t, yb+t, 100-(2*t), level, bcolor);
        }
        // Plus pol 20x15
        int xp = xb + 20;
        int yp = yb - 15 + t;
        getdisplay().fillRect(xp, yp, 20, 15, pcolor);
        getdisplay().fillRect(xp+t, yp+t, 20-(2*t), 15-(2*t), bcolor);
        // Minus pol 20x15
        int xm = xb + 60;
        int ym = yb -15 + t;
        getdisplay().fillRect(xm, ym, 20, 15, pcolor);
        getdisplay().fillRect(xm+t, ym+t, 20-(2*t), 15-(2*t), bcolor);
}

// Solar graphic with fill level
void solarGraphic(uint x, uint y, int pcolor, int bcolor){
    // Show solar modul
        int xb = x;     // X position
        int yb = y;     // Y position
        int t = 4;      // Line thickness
        int percent = 0;
        // Solar corpus 100x80
        int level = int((100.0 - percent) * (80-(2*t)) / 100.0);
        getdisplay().fillRect(xb, yb, 100, 80, pcolor);
        if(percent < 99){
            getdisplay().fillRect(xb+t, yb+t, 100-(2*t), level, bcolor);
        }
        // Draw horizontel lines
        getdisplay().fillRect(xb, yb+28-t, 100, t, pcolor);
        getdisplay().fillRect(xb, yb+54-t, 100, t, pcolor);
        // Draw vertical lines
        getdisplay().fillRect(xb+19+t, yb, t, 80, pcolor);
        getdisplay().fillRect(xb+39+2*t, yb, t, 80, pcolor);
        getdisplay().fillRect(xb+59+3*t, yb, t, 80, pcolor);

}

// Generator graphic with fill level
void generatorGraphic(uint x, uint y, int pcolor, int bcolor){
    // Show battery
        int xb = x;     // X position
        int yb = y;     // Y position
        int t = 4;      // Line thickness

        // Generator corpus with radius 45
        getdisplay().fillCircle(xb, yb, 45, pcolor);
        getdisplay().fillCircle(xb, yb, 41, bcolor);
        // Insert G
        getdisplay().setTextColor(pcolor);
        getdisplay().setFont(&Ubuntu_Bold32pt7b);
        getdisplay().setCursor(xb-22, yb+20);
        getdisplay().print("G");
}

// Function to handle HTTP image request
// http://192.168.15.1/api/user/OBP60Task/screenshot
void doImageRequest(GwApi *api, int *pageno, const PageStruct pages[MAX_PAGE_NUMBER], AsyncWebServerRequest *request) {
    GwLog *logger = api->getLogger();

    String imgformat = api->getConfig()->getConfigItem(api->getConfig()->imageFormat,true)->asString();
    imgformat.toLowerCase();
    String filename = "Page" + String(*pageno) + "_" +  pages[*pageno].description->pageName + "." + imgformat;

    logger->logDebug(GwLog::LOG,"handle image request [%s]: %s", imgformat, filename);

    uint8_t *fb = getdisplay().getBuffer(); // EPD framebuffer
    std::vector<uint8_t> imageBuffer;       // image in webserver transferbuffer
    String mimetype;

    if (imgformat == "gif") {
        // GIF is commpressed with LZW, so small
        mimetype = "image/gif";
        if (!createGIF(fb, &imageBuffer, GxEPD_WIDTH, GxEPD_HEIGHT)) {
             logger->logDebug(GwLog::LOG,"GIF creation failed: Hashtable init error!");
             return;
        }
    }
    else if (imgformat == "bmp") {
        // Microsoft BMP bitmap
        mimetype = "image/bmp";
        createBMP(fb, &imageBuffer, GxEPD_WIDTH, GxEPD_HEIGHT);
    }
    else {
        // PBM simple portable bitmap
        mimetype = "image/x-portable-bitmap";
        createPBM(fb, &imageBuffer, GxEPD_WIDTH, GxEPD_HEIGHT);
    }

    AsyncWebServerResponse *response = request->beginResponse_P(200, mimetype, (const uint8_t*)imageBuffer.data(), imageBuffer.size());
    response->addHeader("Content-Disposition", "inline; filename=" + filename);
    request->send(response);

    imageBuffer.clear();
}

#endif
