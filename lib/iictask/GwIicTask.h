#ifndef _GWIICTASK_H
#define _GWIICTASK_H
#include "GwApi.h"
#include "GwSensor.h"
void initIicTask(GwApi *api);
DECLARE_INITFUNCTION(initIicTask);
class IICSensors : public GwApi::TaskInterfaces::Base{
    public:
    SensorList sensors;
};
DECLARE_TASKIF(IICSensors);
#endif