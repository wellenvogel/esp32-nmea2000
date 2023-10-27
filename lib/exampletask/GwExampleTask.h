#pragma once
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_TEST
//we could add the following defines also in our local platformio.ini
//CAN base 
#define M5_CAN_KIT
//RS485 on groove
#define SERIAL_GROOVE_485

void exampleInit(GwApi *param);

//let the core call an init function before the 
//N2K Stuff and the communication is set up
//normally you should not need this at all
//this function must return when done - otherwise the core will not start up
DECLARE_INITFUNCTION(exampleInit);

#endif