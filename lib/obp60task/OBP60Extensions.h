#ifndef _OBP60EXTENSIONPORT_H
#define _OBP60EXTENSIONPORT_H

#include <Arduino.h>
#include "OBP60Hardware.h"
#define FASTLED_ALL_PINS_HARDWARE_SPI
#define FASTLED_ESP32_SPI_BUS FSPI
#define FASTLED_ESP32_FLASH_LOCK 1
#include "LedSpiTask.h"
#include <GxEPD2_BW.h>                  // E-paper lib V2
#include <Adafruit_FRAM_I2C.h>          // I2C FRAM

// FRAM address reservations 32kB: 0x0000 - 0x7FFF
// 0x0000 - 0x03ff: single variables
#define FRAM_VOLTAGE_AVG 0x000A
#define FRAM_VOLTAGE_TREND 0x000B
#define FRAM_VOLTAGE_MODE 0x000C
// Barograph history data
#define FRAM_BAROGRAPH_START 0x0400
#define FRAM_BAROGRAPH_END 0x13FF

extern Adafruit_FRAM_I2C fram;
extern bool hasFRAM;

// Fonts declarations for display (#inclues see OBP60Extensions.cpp)
extern const GFXfont Ubuntu_Bold8pt7b;
extern const GFXfont Ubuntu_Bold10pt7b;
extern const GFXfont Ubuntu_Bold12pt7b;
extern const GFXfont Ubuntu_Bold16pt7b;
extern const GFXfont Ubuntu_Bold20pt7b;
extern const GFXfont Ubuntu_Bold32pt7b;
extern const GFXfont DSEG7Classic_BoldItalic16pt7b;
extern const GFXfont DSEG7Classic_BoldItalic20pt7b;
extern const GFXfont DSEG7Classic_BoldItalic30pt7b;
extern const GFXfont DSEG7Classic_BoldItalic42pt7b;
extern const GFXfont DSEG7Classic_BoldItalic60pt7b;

// Gloabl functions
#ifdef DISPLAY_GDEW042T2
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> & getdisplay();
#endif

#ifdef DISPLAY_GDEY042T81
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> & getdisplay();
#endif

#ifdef DISPLAY_GYE042A87
GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> & getdisplay();
#endif

#ifdef DISPLAY_SE0420NQ04
GxEPD2_BW<GxEPD2_420_SE0420NQ04, GxEPD2_420_SE0420NQ04::HEIGHT> & getdisplay();
#endif

struct Point {
    double x;
    double y;
};
Point rotatePoint(const Point& origin, const Point& p, double angle);
std::vector<Point> rotatePoints(const Point& origin, const std::vector<Point>& pts, double angle);
void fillPoly4(const std::vector<Point>& p4, uint16_t color);

void hardwareInit(GwApi *api);

void setPortPin(uint pin, bool value);          // Set port pin for extension port

void togglePortPin(uint pin);                   // Toggle extension port pin

Color colorMapping(const String &colorString);          // Color mapping string to CHSV colors
void setBacklightLED(uint brightness, const Color &color);// Set backlight LEDs
void toggleBacklightLED(uint brightness,const Color &color);// Toggle backlight LEDs

void setFlashLED(bool status);                  // Set flash LED
void blinkingFlashLED();                        // Blinking function for flash LED
void setBlinkingLED(bool on);                   // Set blinking flash LED active

void buzzer(uint frequency, uint duration);     // Buzzer function
void setBuzzerPower(uint power);                // Set buzzer power

String xdrDelete(String input);                 // Delete xdr prefix from string

void drawTextCenter(int16_t cx, int16_t cy, String text);
void drawTextRalign(int16_t x, int16_t y, String text);

void displayTrendHigh(int16_t x, int16_t y, uint16_t size, uint16_t color);
void displayTrendLow(int16_t x, int16_t y, uint16_t size, uint16_t color);

void displayHeader(CommonData &commonData, GwApi::BoatValue *date, GwApi::BoatValue *time, GwApi::BoatValue *hdop); // Draw display header

SunData calcSunsetSunrise(GwApi *api, double time, double date, double latitude, double longitude, double timezone); // Calulate sunset and sunrise

void batteryGraphic(uint x, uint y, float percent, int pcolor, int bcolor); // Battery graphic with fill level
void solarGraphic(uint x, uint y, int pcolor, int bcolor);                  // Solar graphic with fill level
void generatorGraphic(uint x, uint y, int pcolor, int bcolor);              // Generator graphic with fill level
void startLedTask(GwApi *api);

void doImageRequest(GwApi *api, int *pageno, const PageStruct pages[MAX_PAGE_NUMBER], AsyncWebServerRequest *request);

#define fram_width 16
#define fram_height 16
static unsigned char fram_bits[] PROGMEM = {
   0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f, 0xf8, 0x1f, 0xff, 0xff,
   0xff, 0xff, 0xf8, 0x1f, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f,
   0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f };

#endif
