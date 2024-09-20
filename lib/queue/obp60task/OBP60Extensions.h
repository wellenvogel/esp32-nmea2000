#ifndef _OBP60EXTENSIONPORT_H
#define _OBP60EXTENSIONPORT_H

#include <Arduino.h>
#include "OBP60Hardware.h"
#define FASTLED_ALL_PINS_HARDWARE_SPI
#define FASTLED_ESP32_SPI_BUS FSPI
#define FASTLED_ESP32_FLASH_LOCK 1
#include "LedSpiTask.h"
#include <GxEPD2_BW.h>                  // E-paper lib V2

// Fonts declarations for display (#inclues see OBP60Extensions.cpp)
extern const GFXfont Ubuntu_Bold8pt7b;
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

void hardwareInit();

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

void displayTrendHigh(int16_t x, int16_t y, uint16_t size, uint16_t color);
void displayTrendLow(int16_t x, int16_t y, uint16_t size, uint16_t color);

void displayHeader(CommonData &commonData, GwApi::BoatValue *date, GwApi::BoatValue *time, GwApi::BoatValue *hdop); // Draw display header

SunData calcSunsetSunrise(GwApi *api, double time, double date, double latitude, double longitude, double timezone); // Calulate sunset and sunrise

void batteryGraphic(uint x, uint y, float percent, int pcolor, int bcolor); // Battery graphic with fill level
void solarGraphic(uint x, uint y, int pcolor, int bcolor);                  // Solar graphic with fill level
void generatorGraphic(uint x, uint y, int pcolor, int bcolor);              // Generator graphic with fill level
void startLedTask(GwApi *api);

#endif