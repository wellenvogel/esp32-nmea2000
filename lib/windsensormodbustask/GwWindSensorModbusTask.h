#ifndef _GWINDSENSORMODBUSTASK_H
#define _GWINDSENSORMODBUSTASK_H
#include "GwApi.h"

void initWindSensorModbusTask(GwApi *api);
DECLARE_INITFUNCTION(initWindSensorModbusTask);

void runWindSensorModbusTask(GwApi *api);

#endif // _GWINDSENSORMODBUSTASK_H
