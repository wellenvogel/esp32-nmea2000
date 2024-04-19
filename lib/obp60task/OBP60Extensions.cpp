#ifdef BOARD_OBP60S3

#include <Arduino.h>
#include <FastLED.h>      // Driver for WS2812 RGB LED
#include <PCF8574.h>      // Driver for PCF8574 output modul from Horter
#include <Wire.h>         // I2C
#include <RTClib.h>       // Driver for DS1388 RTC
#include "SunRise.h"      // Lib for sunrise and sunset calculation
#include "Pagedata.h"
#include "OBP60Hardware.h"
#include "OBP60Extensions.h"

// Please dont forget to declarate the fonts in OBP60ExtensionPort.h
#include "Ubuntu_Bold8pt7b.h"
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

#ifdef DISPLAY_GYE042A8
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

// Define the array of leds
CRGB fled[NUM_FLASH_LED];           // Flash LED
CRGB backlight[NUM_BACKLIGHT_LED];  // Backlight

// Global vars
bool blinkingLED = false;       // Enable / disable blinking flash LED
bool statusLED = false;         // Actual status of flash LED on/off
bool statusBacklightLED = false;// Actual status of flash LED on/off

int uvDuration = 0;             // Under voltage duration in n x 100ms

void hardwareInit()
{
    // Init power rail 5.0V
    setPortPin(OBP_POWER_50, true);

    // Init RGB LEDs
    FastLED.addLeds<WS2812B, OBP_FLASH_LED, GRB>(fled, NUM_FLASH_LED);
    FastLED.addLeds<WS2812B, OBP_BACKLIGHT_LED, GRB>(backlight, NUM_BACKLIGHT_LED);

    // Init PCF8574 digital outputs
    Wire.setClock(I2C_SPEED);       // Set I2C clock on 10 kHz
    if(pcf8574_Out.begin()){        // Initialize PCF8574
        pcf8574_Out.write8(255);    // Clear all outputs
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

// Valid colors see hue
CHSV colorMapping(String colorString){
    CHSV color = CHSV(HUE_RED, 255, 255);
    if(colorString == "Red"){color = CHSV(HUE_RED, 255, 255);}
    if(colorString == "Orange"){color = CHSV(HUE_ORANGE, 255, 255);}
    if(colorString == "Yellow"){color = CHSV(HUE_YELLOW, 255, 255);}
    if(colorString == "Green"){color = CHSV(HUE_GREEN, 255, 255);}
    if(colorString == "Blue"){color = CHSV(HUE_BLUE, 255, 255);}
    if(colorString == "Aqua"){color = CHSV(HUE_AQUA, 255, 255);}
    if(colorString == "Violet"){color = CHSV(HUE_PURPLE, 255, 255);}
    if(colorString == "White"){color = CHSV(HUE_AQUA, 0, 255);}
    return color;
}

// All defined colors see pixeltypes.h in FastLED lib
void setBacklightLED(uint brightness, CHSV color){
    FastLED.setBrightness(255); // Brightness for flash LED
    color.value = brightness;
    backlight[0] = color;       // Backlight LEDs on with color
    backlight[1] = color;
    backlight[2] = color;
    backlight[3] = color;
    backlight[4] = color;
    backlight[5] = color;
    FastLED.show(); 
}

void toggleBacklightLED(uint brightness, CHSV color){
    statusBacklightLED = !statusBacklightLED;
    FastLED.setBrightness(255); // Brightness for flash LED
    if(statusBacklightLED == true){
        color.value = brightness;
        backlight[0] = color;   // Backlight LEDs on with color
        backlight[1] = color;
        backlight[2] = color;
        backlight[3] = color;
        backlight[4] = color;
        backlight[5] = color;
    }
    else{
        backlight[0] = CHSV(HUE_BLUE, 255, 0); // Backlight LEDs off (blue without britghness)
        backlight[1] = CHSV(HUE_BLUE, 255, 0);
        backlight[2] = CHSV(HUE_BLUE, 255, 0);
        backlight[3] = CHSV(HUE_BLUE, 255, 0);
        backlight[4] = CHSV(HUE_BLUE, 255, 0);
        backlight[5] = CHSV(HUE_BLUE, 255, 0);
    }
    FastLED.show(); 
}

void setFlashLED(bool status){
    statusLED = status;
    FastLED.setBrightness(255); // Brightness for flash LED
    if(statusLED == true){
        fled[0] = CRGB::Red;    // Flash LED on in red
    }
    else{
        fled[0] = CRGB::Black;  // Flash LED off
    }
    FastLED.show();  
}

void blinkingFlashLED(){
    if(blinkingLED == true){
        statusLED = !statusLED;
        FastLED.setBrightness(255); // Brightness for flash LED
        if(statusLED == true){
            fled[0] = CRGB::Red;    // Flash LED on in red
        }
        else{
            fled[0] = CRGB::Black;  // Flash LED off
        }
        FastLED.show();
    }    
}

void setBlinkingLED(bool on){
    blinkingLED = on;
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

// Show a triangle for trend direction high (x, y is the left edge)
void displayTrendHigh(int16_t x, int16_t y, uint16_t size, uint16_t color){
    getdisplay().fillTriangle(x, y, x+size*2, y, x+size, y-size*2, color);
}

// Show a triangle for trend direction low (x, y is the left edge)
void displayTrendLow(int16_t x, int16_t y, uint16_t size, uint16_t color){
    getdisplay().fillTriangle(x, y, x+size*2, y, x+size, y+size*2, color);
}

// Show header informations
void displayHeader(CommonData &commonData, GwApi::BoatValue *date, GwApi::BoatValue *time){

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
        if(commonData.config->getBool(commonData.config->useGPS) == true && date->valid == true){
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

#endif