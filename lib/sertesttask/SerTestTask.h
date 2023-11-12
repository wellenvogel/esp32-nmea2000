#ifndef _SERTESTTASK_H
#define _SERTESTTASK_H
#include "GwApi.h"
#ifdef SERTEST
void sertestInit(GwApi *param);
DECLARE_INITFUNCTION(sertestInit);
#define M5_CAN_KIT
/*used ATOM pins
22,19,23,33,21,25 GR 26,32
*/
#define GWSERIAL_RX 13
#define GWSERIAL_TYPE GWSERIAL_TYPE_RX
#define CFGDEFAULT_serialBaud "9600"
#endif
#endif