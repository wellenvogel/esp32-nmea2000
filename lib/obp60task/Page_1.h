#ifndef _PAGE_1_H
#define _PAGE_1_H

#include <Arduino.h>
#include "OBP60Hardware.h"

void page_1(){
  // Measuring Values
  display.setFont(&Ubuntu_Bold32pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(20, 100);
  display.print("Speed");
  display.setFont(&Ubuntu_Bold20pt7b);
  display.setCursor(270, 100);
  display.print("km/h");
  display.setFont(&DSEG7Classic_BoldItalic60pt7b);
  display.setCursor(20, 240);
  float speed = 5;
  speed += float(random(0, 13)) / 10;
  display.print(speed,1);

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