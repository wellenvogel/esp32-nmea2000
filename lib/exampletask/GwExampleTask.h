#ifndef _GWEXAMPLETASK_H
#define _GWEXAMPLETASK_H
#include "GwExampleHardware.h"
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_TEST
void exampleTask(void *param);
//make the task known to the core
DECLARE_USERTASK(exampleTask);
#endif
#endif