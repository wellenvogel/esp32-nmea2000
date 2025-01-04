#ifdef BOARD_OBP60S3

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

void startLedTask(GwApi *api){
    ledTaskData=new LedTaskData(api);
    createSpiLedTask(ledTaskData);
}

void setPortPin(uint pin, bool value){
    pinMode(pin, OUTPUT);
    digitalWrite(pin, value);
}

void togglePortPin(uint pin){
    pinMode(pin, OUTPUT);
    digitalWrite(pin, !digitalRead(pin));
}

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
    int textcolor = GxEPD_BLACK;

    if(commonData.config->getBool(commonData.config->statusLine) == true){

        if(commonData.config->getString(commonData.config->displaycolor) == "Normal"){
            textcolor = GxEPD_BLACK;
        }
        else{
            textcolor = GxEPD_WHITE;
        }

        // Show status info
        getdisplay().setTextColor(textcolor);
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

        // Heartbeat as dot
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold32pt7b);
        getdisplay().setCursor(205, 14);
        if(heartbeat == true){
        getdisplay().print(".");
        }
        else{
        getdisplay().print(" ");
        }
        heartbeat = !heartbeat; 

        // Date and time
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(230, 15);
        // Show date and time if date present
        if(date->valid == true){
            String acttime = formatValue(time, commonData).svalue;
            acttime = acttime.substring(0, 5);
            String actdate = formatValue(date, commonData).svalue;
            getdisplay().print(acttime);
            getdisplay().print(" ");
            getdisplay().print(actdate);
            getdisplay().print(" ");
            if(commonData.config->getInt(commonData.config->timeZone) == 0){
                getdisplay().print("UTC");
            }
            else{
                getdisplay().print("LOT");
            }
        }
        else{
            if(commonData.config->getBool(commonData.config->useSimuData) == true){
                getdisplay().print("12:00 01.01.2024 LOT");
            }
            else{
                getdisplay().print("No GPS data");
            }
        }
    }
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
        getdisplay().print("G");Function to handle HTTP image request
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
