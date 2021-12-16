#ifndef _GWOBP60TASK_H
#define _GWOBP60TASK_H
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_NODEMCU32S_OBP60
    //CAN NMEA2000
    #define ESP32_CAN_TX_PIN GPIO_NUM_13
    #define ESP32_CAN_RX_PIN GPIO_NUM_12
    //RS485 NMEA0183
    #define GWSERIAL_TX 26
    #define GWSERIAL_RX 14
    #define GWSERIAL_MODE "UNI"

// Init OBP60 Task
void OBP60Init(GwApi *param);
DECLARE_INITFUNCTION(OBP60Init);

// OBP60 Task
void OBP60Task(void *param);
DECLARE_USERTASK_PARAM(OBP60Task, 25000)    // Need 25k RAM as stack size
DECLARE_CAPABILITY(obp60,true);
#endif
#endif
