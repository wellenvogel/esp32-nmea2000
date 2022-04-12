#ifdef BOARD_NODEMCU32S_OBP60

#include <Arduino.h>
#include "SunRise.h"                    // Lib for sunrise and sunset calculation
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

MCP23017 mcp = MCP23017(MCP23017_I2C_ADDR);

// SPI pin definitions for E-Ink display
GxIO_Class io(SPI, OBP_SPI_CS, OBP_SPI_DC, OBP_SPI_RST);  // SPI, CS, DC, RST
GxEPD_Class display(io, OBP_SPI_RST, OBP_SPI_BUSY);       // io, RST, BUSY

// Global vars
int outA = 0;                   // Outport Byte A
int outB = 0;                   // Outport Byte B
bool blinkingLED = false;       // Enable / disable blinking flash LED
int uvDuration = 0;             // Under voltage duration in n x 100ms

void MCP23017Init()
{
    mcp.init();
    mcp.portMode(MCP23017Port::A, 0b00110000); // Port A, 0 = out, 1 = in
    mcp.portMode(MCP23017Port::B, 0b11110000); // Port B, 0 = out, 1 = in

    // Extension Port A set defaults
    setPortPin(OBP_DIGITAL_OUT1, false);  // PA0
    setPortPin(OBP_DIGITAL_OUT2, false);  // PA1
    setPortPin(OBP_FLASH_LED, false);     // PA2
    setPortPin(OBP_BACKLIGHT_LED, false); // PA3
    setPortPin(OBP_POWER_50, true);       // PA6
    setPortPin(OBP_POWER_33, true);       // PA7

    // Extension Port B set defaults
    setPortPin(PB0, false); // PB0
    setPortPin(PB1, false); // PB1
    setPortPin(PB2, false); // PB2
    setPortPin(PB3, false); // PB3
}

void setPortPin(uint pin, bool value){
    if(pin <=7){    
        outA &= ~(1 << pin);     // Clear bit    
        outA |= (value << pin);  // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_A, outA);
    }
    else{
        pin = pin - 8;
        outB &= ~(1 << pin);     // Clear bit
        outB |= (value << pin);  // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_B, outB);
    }
}

void togglePortPin(uint pin){
    if(pin <=7){
        outA ^= (1 << pin);     // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_A, outA);
    }
    else{
        pin = pin - 8;
        outB ^= (1 << pin);     // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_B, outB);
    }
}

void blinkingFlashLED(){
    if(blinkingLED == true){
        togglePortPin(OBP_FLASH_LED);
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
    if(buzzerpower > 100){        // Max 100%
        buzzerpower = 100;
    }
    if(duration > 1000){    // Max 1000ms
        duration = 1000;
    }
    
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
    display.fillTriangle(x, y, x+size*2, y, x+size, y-size*2, color);
}

// Show a triangle for trend direction low (x, y is the left edge)
void displayTrendLow(int16_t x, int16_t y, uint16_t size, uint16_t color){
    display.fillTriangle(x, y, x+size*2, y, x+size, y+size*2, color);
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
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(0, 15);
        if(commonData.status.wifiApOn){
        display.print(" AP ");
        }
        // If receive new telegram data then display bus name
        if(commonData.status.tcpClRx != tcpClRxOld || commonData.status.tcpClTx != tcpClTxOld || commonData.status.tcpSerRx != tcpSerRxOld || commonData.status.tcpSerTx != tcpSerTxOld){
        display.print("TCP ");
        }
        if(commonData.status.n2kRx != n2kRxOld || commonData.status.n2kTx != n2kTxOld){
        display.print("N2K ");
        }
        if(commonData.status.serRx != serRxOld || commonData.status.serTx != serTxOld){
        display.print("183 ");
        }
        if(commonData.status.usbRx != usbRxOld || commonData.status.usbTx != usbTxOld){
        display.print("USB ");
        }
        if(commonData.config->getBool(commonData.config->useGPS) == true && date->valid == true){
        display.print("GPS");
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
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold32pt7b);
        display.setCursor(205, 14);
        if(heartbeat == true){
        display.print(".");
        }
        else{
        display.print(" ");
        }
        heartbeat = !heartbeat; 

        // Date and time
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(230, 15);
        if(date->valid == true){
            String acttime = formatValue(time, commonData).svalue;
            acttime = acttime.substring(0, 5);
            String actdate = formatValue(date, commonData).svalue;
            display.print(acttime);
            display.print(" ");
            display.print(actdate);
            display.print(" ");
            if(commonData.config->getInt(commonData.config->timeZone) == 0){
                display.print("UTC");
            }
            else{
                display.print("LOT");
            }
        }
        else{
        display.print("No GPS data");
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
        display.fillRect(xb, yb, 100, 80, pcolor);
        if(percent < 99){
            display.fillRect(xb+t, yb+t, 100-(2*t), level, bcolor);
        }
        // Plus pol 20x15
        int xp = xb + 20;
        int yp = yb - 15 + t;
        display.fillRect(xp, yp, 20, 15, pcolor);
        display.fillRect(xp+t, yp+t, 20-(2*t), 15-(2*t), bcolor);
        // Minus pol 20x15
        int xm = xb + 60;
        int ym = yb -15 + t;
        display.fillRect(xm, ym, 20, 15, pcolor);
        display.fillRect(xm+t, ym+t, 20-(2*t), 15-(2*t), bcolor);
}

#endif