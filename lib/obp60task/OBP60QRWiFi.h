#ifndef _OBP60QRWIFI_H
#define _OBP60QRWIFI_H

#include <Arduino.h>
#include "qrcode.h"
  
void qrWiFi(String ssid, String passwd, bool refreshmode){
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
        display.fillRect(box_x, box_y, box_s, box_s, GxEPD_BLACK);
      } else {
        display.fillRect(box_x, box_y, box_s, box_s, GxEPD_WHITE);
      }
      box_x = box_x + box_s;
    }
    box_y = box_y + box_s;
    box_x = init_x;
  }
  display.setFont(&Ubuntu_Bold32pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(140, 285);
  display.print("WiFi");
  if(refreshmode == true){
    display.update();       // Full update (slow)
  }
  else{
    display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);   // Partial update (fast)
  }
}

#endif