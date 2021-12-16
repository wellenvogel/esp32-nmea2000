#ifndef _PAGE_0_H
#define _PAGE_0_H

#include <Arduino.h>
#include "OBP60Hardware.h"

void page_0(){
  // Measuring Values
  display.setFont(&Ubuntu_Bold32pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(20, 100);
  display.print("Depth");
  display.setFont(&Ubuntu_Bold20pt7b);
  display.setCursor(270, 100);
  display.print("m");
  display.setFont(&DSEG7Classic_BoldItalic60pt7b);
  display.setCursor(20, 240);
  float depth = 84;
  depth += float(random(0, 13)) / 10;
  display.print(depth,1);

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