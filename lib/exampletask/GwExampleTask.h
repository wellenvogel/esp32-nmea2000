#ifndef _GWEXAMPLETASK_H
#define _GWEXAMPLETASK_H
#include "GwExampleHardware.h"
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_TEST
void exampleTask(void *param);
//make the task known to the core
DECLARE_USERTASK(exampleTask);
//we declare a capability that we can
//use in config.json to only show some
//elements when this capability is set correctly
DECLARE_CAPABILITY(testboard,true);
#endif
#endif