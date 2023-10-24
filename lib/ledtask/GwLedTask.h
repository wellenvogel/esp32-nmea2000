#ifndef _GWLEDS_H
#define _GWLEDS_H
#include "GwApi.h"
//task function
void handleLeds(GwApi *param);

DECLARE_USERTASK(handleLeds);
#endif