#ifndef _SERTESTTASK_H
#define _SERTESTTASK_H
#include "GwApi.h"
#ifdef SERTEST
void sertestInit(GwApi *param);
DECLARE_INITFUNCTION(sertestInit);
#define M5_CAN_KIT
#define GWSERIAL_RX GROOVE_PIN_1
#define GWSERIAL_TYPE GWSERIAL_TYPE_RX
#define CFGDEFAULT_serialBaud "9600"
#endif
#endif