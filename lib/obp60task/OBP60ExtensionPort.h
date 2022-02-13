#ifndef _OBP60EXTENSIONPORT_H
#define _OBP60EXTENSIONPORT_H

#include <Arduino.h>
#include "OBP60Hardware.h"
#include <MCP23017.h>
#include <GxGDEW042T2/GxGDEW042T2.h>    // 4.2" Waveshare S/W 300 x 400 pixel
#include <GxIO/GxIO_SPI/GxIO_SPI.h>     // GxEPD lip for SPI display communikation
#include <GxIO/GxIO.h>                  // GxEPD lip for SPI

extern MCP23017 mcp;

#define buzPower 50                     // Buzzer loudness in [%]

// SPI pin definitions for E-Ink display
extern GxIO_Class io;
extern GxEPD_Class display;

void setPortPin(uint pin, bool value);

void togglePortPin(uint pin);
void blinkingFlashLED();

void buzzer(uint frequency, uint power, uint duration);

void underVoltageDetection();

void setBlinkingLED(bool on);

#endif