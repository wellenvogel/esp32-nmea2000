#ifndef _OBP60PAGES_H
#define _OBP60PAGES_H

#include <Arduino.h>

// Global vars
int pageNumber = 0;           // Page number for actual page
bool first_view = true;
bool heartbeat = false;
unsigned long usbRxOld = 0;
unsigned long usbTxOld = 0;
unsigned long serRxOld = 0;
unsigned long serTxOld = 0;
unsigned long tcpSerRxOld = 0;
unsigned long tcpSerTxOld = 0;
unsigned long tcpClRxOld = 0;
unsigned long tcpClTxOld = 0;
unsigned long n2kRxOld = 0;
unsigned long n2kTxOld = 0;

void showPage(busData values){
  // Clear display
  display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);   // Draw white sreen

  if(values.statusline == true){
    // Print status info
    display.setFont(&Ubuntu_Bold8pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(0, 15);
    if(values.wifiApOn){
      display.print(" AP ");
    }
    // If receive new telegram data then display bus name
    if(values.tcpClRx != tcpClRxOld || values.tcpClTx != tcpClTxOld || values.tcpSerRx != tcpSerRxOld || values.tcpSerTx != tcpSerTxOld){
      display.print("TCP ");
    }
    if(values.n2kRx != n2kRxOld || values.n2kTx != n2kTxOld){
      display.print("N2K ");
    }
    if(values.serRx != serRxOld || values.serTx != serTxOld){
      display.print("183 ");
    }
    if(values.usbRx != usbRxOld || values.usbTx != usbTxOld){
      display.print("USB ");
    }
    if(values.gps == true && values.PDOP.valid == true && values.PDOP.fvalue <= 50){
     display.print("GPS");
    }
    // Save old telegram counter
    tcpClRxOld = values.tcpClRx;
    tcpClTxOld = values.tcpClTx;
    tcpSerRxOld = values.tcpSerRx;
    tcpSerTxOld = values.tcpSerTx;
    n2kRxOld = values.n2kRx;
    n2kTxOld = values.n2kTx;
    serRxOld = values.serRx;
    serTxOld = values.serTx;
    usbRxOld = values.usbRx;
    usbTxOld = values.usbTx;

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
    if(values.HDOP.valid == true && values.HDOP.fvalue <= 50){
      char newdate[16] = "";
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
      if(values.timezone == 0){
        display.print(values.Time.svalue);
        display.print(" ");
        display.print("UTC");
      }
      else{
        char newtime[16] = "";
        char newhour[3] = "";
        int hour = 0;
        strcpy(newtime, values.Time.svalue);    
        newhour[0] = values.Time.svalue[0];
        newhour[1] = values.Time.svalue[1];
        hour = strtol(newhour, 0, 10);
        if(values.timezone > 0){
          hour += values.timezone;
        }
        else{
          hour += values.timezone + 24;
        }
        hour %= 24;
        sprintf(newhour, "%02d", hour);
        newtime[0] = newhour[0];
        newtime[1] = newhour[1];
        display.print(newtime);
        display.print(" ");
        display.print("LOT");
      }
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
    if(first_view == true){
      display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Needs partial update before full update to refresh the frame buffer
      display.update(); // Full update
    }
    else{
      display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)
    }
  }  
  else{
    display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)
  }
  
  first_view = false;
}

#endif