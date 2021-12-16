#ifndef _PAGE_2_H
#define _PAGE_2_H

#include <Arduino.h>
#include "OBP60Hardware.h"

void page_2(){
  // Measuring Values
  display.setFont(&Ubuntu_Bold32pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(20, 100);
  display.print("VBat");
  display.setFont(&Ubuntu_Bold20pt7b);
  display.setCursor(270, 100);
  display.print("V");
  display.setFont(&DSEG7Classic_BoldItalic60pt7b);
  display.setCursor(20, 240);
  float actVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096) * 20;   // Vin = 1/20 
  display.print(actVoltage,1);

  // Key Layout
  display.setFont(&Ubuntu_Bold8pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 290);
  display.print(" [  <  ]");
  display.setCursor(290, 290);
  display.print("[  >  ]");
  display.setCursor(343, 290);
  display.print("[ILUM]");
}

#endif