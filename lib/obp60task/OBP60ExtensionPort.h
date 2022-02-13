#ifndef _OBP60EXTENSIONPORT_H
#define _OBP60EXTENSIONPORT_H

#include <Arduino.h>
#include "OBP60Hardware.h"
#include <MCP23017.h>
#include <GxGDEW042T2/GxGDEW042T2.h>    // 4.2" Waveshare S/W 300 x 400 pixel
#include <GxIO/GxIO_SPI/GxIO_SPI.h>     // GxEPD lip for SPI display communikation
#include <GxIO/GxIO.h>                  // GxEPD lip for SPI

void MCP23017Init();

// SPI pin definitions for E-Ink display
extern GxEPD_Class display;
extern const GFXfont Ubuntu_Bold8pt7b;
extern const GFXfont Ubuntu_Bold20pt7b;
extern const GFXfont Ubuntu_Bold32pt7b;
extern const GFXfont DSEG7Classic_BoldItalic16pt7b;
extern const GFXfont DSEG7Classic_BoldItalic42pt7b;
extern const GFXfont DSEG7Classic_BoldItalic60pt7b;

void setPortPin(uint pin, bool value);

void togglePortPin(uint pin);

void blinkingFlashLED();

void buzzer(uint frequency, uint duration);
void setBuzzerPower(uint power);

void underVoltageDetection();

void setBlinkingLED(bool on);

#endif