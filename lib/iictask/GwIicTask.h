#ifndef _GWIICTASK_H
#define _GWIICTASK_H
#include "GwApi.h"
#include "GwSensor.h"
void initIicTask(GwApi *api);
DECLARE_INITFUNCTION(initIicTask);
#endif