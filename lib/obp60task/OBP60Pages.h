#ifndef _OBP60PAGES_H
#define _OBP60PAGES_H

#include <Arduino.h>

// Global vars
int pageNumber = 0;           // Page number for actual page
bool first_view = true;
bool heartbeat = false;

void showPage(busData values){
  // Clear display
  display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);   // Draw white sreen

  if(values.statusline == true){
    // Print status info
    display.setFont(&Ubuntu_Bold8pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(0, 15);
    display.print(" WiFi AP TCP N2K 183 ");
    if(values.PDOP.valid == true && values.PDOP.fvalue <= 50){
     display.print("GPS");
    }

    // Heartbeat as dot
    display.setFont(&Ubuntu_Bold32pt7b);
    display.setCursor(205, 14);
    if(heartbeat == true){
      display.print(".");
    }
    else{
      display.print(" ");
    }
    heartbeat = !heartbeat; 

    // Date and time
    display.setFont(&Ubuntu_Bold8pt7b);
    display.setCursor(230, 15);
    char newdate[16] = "";
    if(values.PDOP.valid == true && values.PDOP.fvalue <= 50){
      if(String(values.dateformat) == "DE"){
        display.print(values.Date.svalue);
      }
      if(String(values.dateformat) == "GB"){
        values.Date.svalue[2] = '/';
        values.Date.svalue[5] = '/';
        display.print(values.Date.svalue);
      }
      if(String(values.dateformat) == "US"){
        char newdate[16] = "";
        strcpy(newdate, values.Date.svalue);
        newdate[0] = values.Date.svalue[3];
        newdate[1] = values.Date.svalue[4];
        newdate[2] = '/';
        newdate[3] = values.Date.svalue[0];
        newdate[4] = values.Date.svalue[1];
        newdate[5] = '/';
        display.print(newdate);
      }
      display.print(" ");
      display.print(values.Time.svalue);
      display.print(" ");
      display.print("UTC");
    }
    else{
      display.print("No GPS data");
    }
  }

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

  // Update display
  if(values.refresh == true){
    display.update(); // Full update
  }
  else{
    display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)
  }
  
  first_view = false;
}

#endif