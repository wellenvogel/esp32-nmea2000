#ifndef _OBP60PAGES_H
#define _OBP60PAGES_H

#include <Arduino.h>

// Global vars
int pageNumber = 0;           // Page number for actual page
bool first_view = true;

void showPage(busData values){
  // Clear display
  display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);   // Draw white sreen

  // Draw status heder
  // display.fillRect(0, 0, GxEPD_WIDTH, 20, GxEPD_BLACK);   // Draw black  box
  display.setFont(&Ubuntu_Bold8pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 15);
  display.print(" WiFi AP TCP N2K 183 GPS");
  display.setCursor(230, 15);
  display.print("14.12.2021 17:50 UTC");

  // Read page number
  switch (pageNumber) {
    case 0:
      page_0(values);
      break;
    case 1:
      page_1();
      break;
    case 2:
      page_2();
      break;
    case 3:
      page_3();
      break;
    case 4:
      // Statement(s)
      break;
    case 5:
      // Statement(s)
      break;    
    default:
        page_0(values);
      break;
  }

  // Partial update display
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)
  
  first_view = false;
}

#endif