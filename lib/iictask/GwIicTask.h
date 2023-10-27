#ifndef _GWIICTASK_H
#define _GWIICTASK_H
#include "GwApi.h"
void runIicTask(GwApi *api);
void initIicTask(GwApi *api);
DECLARE_USERTASK_PARAM(runIicTask,3000);
DECLARE_INITFUNCTION(initIicTask);
#endif