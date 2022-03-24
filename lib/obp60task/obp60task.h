#pragma once
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_NODEMCU32S_OBP60
// CAN NMEA2000
#define ESP32_CAN_TX_PIN GPIO_NUM_13
#define ESP32_CAN_RX_PIN GPIO_NUM_12
// Bus load in 50mA steps
#define N2K_LOAD_LEVEL 5 // 5x50mA = 250mA max bus load with back light on
// RS485 NMEA0183
#define GWSERIAL_TX 26
#define GWSERIAL_RX 14
#define GWSERIAL_MODE "UNI"
// Allowed to set a new password for access point
#define FORCE_AP_PWCHANGE

// Init OBP60 Task
void OBP60Init(GwApi *param);
DECLARE_INITFUNCTION(OBP60Init);

// OBP60 Task
void OBP60Task(GwApi *param);
DECLARE_USERTASK_PARAM(OBP60Task, 20000)    // Need 20k RAM as stack size
DECLARE_CAPABILITY(obp60,true);

#endif