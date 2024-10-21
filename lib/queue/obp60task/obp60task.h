#pragma once
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_OBP60S3
    #define USBSerial Serial
    // CAN NMEA2000
    #define ESP32_CAN_TX_PIN 46
    #define ESP32_CAN_RX_PIN 3
    // Bus load in 50mA steps
    #define N2K_LOAD_LEVEL 5 // 5x50mA = 250mA max bus load with back light on
    // RS485 NMEA0183
    #define GWSERIAL_TX 17
    #define GWSERIAL_RX 8
    #define GWSERIAL_MODE "UNI"
    // Allowed to set a new password for access point
    #define FORCE_AP_PWCHANGE

    // Init OBP60 Task
    void OBP60Init(GwApi *param);
    DECLARE_INITFUNCTION(OBP60Init);

    // OBP60 Task
    void OBP60Task(GwApi *param);
    DECLARE_USERTASK_PARAM(OBP60Task, 10000);   // Need 8k RAM as stack size
    DECLARE_CAPABILITY(obp60,true);
    DECLARE_STRING_CAPABILITY(HELP_URL, "https://obp60-v2-docu.readthedocs.io/de/latest/"); // Link to help pages
#endif