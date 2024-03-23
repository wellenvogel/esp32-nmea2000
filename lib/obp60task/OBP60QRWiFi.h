#ifndef _OBP60QRWIFI_H
#define _OBP60QRWIFI_H

#include <Arduino.h>
#include "OBP60Extensions.h"
#include "qrcode.h"
  
void qrWiFi(String ssid, String passwd, String displaycolor){
  // Set display color
  int textcolor = GxEPD_BLACK;
  int pixelcolor = GxEPD_BLACK;
  int bgcolor = GxEPD_WHITE;
  if(displaycolor == "Normal"){
      textcolor = GxEPD_BLACK;
      pixelcolor = GxEPD_BLACK;
      bgcolor = GxEPD_WHITE;
  }
  else{
      textcolor = GxEPD_WHITE;
      pixelcolor = GxEPD_WHITE;
      bgcolor = GxEPD_BLACK;
  }

  // Set start point and pixel size
  int16_t box_x = 100;      // X offset
  int16_t box_y = 30;       // Y offset
  int16_t box_s = 6;        // Pixel size
  int16_t init_x = box_x;

  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(4)];
  // Content for QR code: "WIFI:S:mySSID;T:WPA;P:myPASSWORD;;"
  String text = "WIFI:S:" + String(ssid) + ";T:WPA;P:" + String(passwd) + ";;";
  const char *qrcodecontent = text.c_str();
  qrcode_initText(&qrcode, qrcodeData, 4, 0, qrcodecontent);

  // Top quiet zone
  for (uint8_t y = 0; y < qrcode.size; y++) {
    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if(qrcode_getModule(&qrcode, x, y)){
        getdisplay().fillRect(box_x, box_y, box_s, box_s, pixelcolor);
      } else {
        getdisplay().fillRect(box_x, box_y, box_s, box_s, bgcolor);
      }
      box_x = box_x + box_s;
    }
    box_y = box_y + box_s;
    box_x = init_x;
  }
  getdisplay().setFont(&Ubuntu_Bold32pt7b);
  getdisplay().setTextColor(textcolor);
  getdisplay().setCursor(140, 285);
  getdisplay().print("WiFi");
  getdisplay().nextPage();                 // Full Refresh
}

#endif