#ifndef _PAGE_0_H
#define _PAGE_0_H

#include <Arduino.h>
#include "OBP60Hardware.h"

void page_0(busData pvalues){
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

  // Reading bus data or using simulation data
  float depth = 0;
  if(pvalues.simulation == true){
    depth = 84;
    depth += float(random(0, 120)) / 10;      // Simulation data
    display.print(depth,1);
  }
  else{
    if(pvalues.WaterDepth.valid == true){     // Check vor valid real data
      depth = pvalues.WaterDepth.fvalue;      // Real bus data
      if(depth <= 99.9){
        display.print(depth,1);
      }
      else{
        display.print(depth,0);
      }
    }
    else{
      display.print("---");                   // Missing bus data
    }  
  }

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