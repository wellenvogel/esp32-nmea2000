#ifndef _OBP60EXTENSIONPORT_H
#define _OBP60EXTENSIONPORT_H

#include <Arduino.h>
#include "OBP60Hardware.h"
#include <MCP23017.h>
#include <FastLED.h>                    // Driver for WS2812 RGB LED
#include <GxGDEW042T2/GxGDEW042T2.h>    // 4.2" Waveshare S/W 300 x 400 pixel
#include <GxIO/GxIO_SPI/GxIO_SPI.h>     // GxEPD lip for SPI display communikation
#include <GxIO/GxIO.h>                  // GxEPD lip for SPI

// E-Ink display
extern GxEPD_Class display;                     // E-Ink display functions

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
void hardwareInit();

void setPortPin(uint pin, bool value);          // Set port pin for extension port

void togglePortPin(uint pin);                   // Toggle extension port pin

void toggleBacklightLED();                      // Toggle backlight LEDs

void setFlashLED(bool status);                  // Set flash LED
void blinkingFlashLED();                        // Blinking function for flash LED
void setBlinkingLED(bool on);                   // Set blinking flash LED active

void buzzer(uint frequency, uint duration);     // Buzzer function
void setBuzzerPower(uint power);                // Set buzzer power

String xdrDelete(String input);                 // Delete xdr prefix from string

void displayTrendHigh(int16_t x, int16_t y, uint16_t size, uint16_t color);
void displayTrendLow(int16_t x, int16_t y, uint16_t size, uint16_t color);

void displayHeader(CommonData &commonData, GwApi::BoatValue *date, GwApi::BoatValue *time); // Draw display header

SunData calcSunsetSunrise(GwApi *api, double time, double date, double latitude, double longitude, double timezone); // Calulate sunset and sunrise

void batteryGraphic(uint x, uint y, float percent, int pcolor, int bcolor); // Battery graphic with fill level

#endif