#pragma once
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_TEST
//we could add the following defines also in our local platformio.ini
//CAN base 
#define M5_CAN_KIT
//RS485 on groove
#define SERIAL_GROOVE_485

void exampleTask(GwApi *param);
void exampleInit(GwApi *param);
//make the task known to the core
//the task function should not return (unless you delete the task - see example code)
//DECLARE_USERTASK(exampleTask)
//if your task is not happy with the default 2000 bytes of stack, replace the DECLARE_USERTASK
DECLARE_USERTASK_PARAM(exampleTask,4000);
//this would create our task with a stack size of 4000 bytes


//let the core call an init function before the 
//N2K Stuff and the communication is set up
//normally you should not need this at all
//this function must return when done - otherwise the core will not start up
DECLARE_INITFUNCTION(exampleInit);
//we declare a capability that we can
//use in config.json to only show some
//elements when this capability is set correctly
DECLARE_CAPABILITY(testboard,true);
DECLARE_CAPABILITY(testboard2,true);
//hide some config value
//just set HIDE + the name of the config item to true
DECLARE_CAPABILITY(HIDEminXdrInterval,true);
//example for a user defined help url that will be shown when clicking the help button
DECLARE_STRING_CAPABILITY(HELP_URL,"https://www.wellenvogel.de");
#endif