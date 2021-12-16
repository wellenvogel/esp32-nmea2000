#ifndef _PAGE_3_H
#define _PAGE_3_H

#include <Arduino.h>
#include "OBP60Hardware.h"

void page_3(){
  // Measuring Values 1
  display.setFont(&Ubuntu_Bold20pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(20, 80);
  display.print("Depth");
  display.setFont(&Ubuntu_Bold20pt7b);
  display.setCursor(20, 130);
  display.print("m");
  display.setFont(&DSEG7Classic_BoldItalic42pt7b);
  display.setCursor(180, 130);
  float depth = 84;
  depth += float(random(0, 13)) / 10;
  display.print(depth,1);

  // Horizontal line 3 pix
  display.fillRect(0, 145, 400, 3, GxEPD_BLACK); // Draw white sreen

  // Measuring Values 2
  display.setFont(&Ubuntu_Bold20pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(20, 190);
  display.print("Speed");
  display.setFont(&Ubuntu_Bold20pt7b);
  display.setCursor(20, 240);
  display.print("kn");
  display.setFont(&DSEG7Classic_BoldItalic42pt7b);
  display.setCursor(180, 240);
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