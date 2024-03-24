#ifndef _GWBME280_H
#define _GWBME280_H
#include "GwIicSensors.h"
IICSensorBase::Creator registerBME280(GwApi *api,IICSensorList &sensors);
#endif