#ifndef _GWSPILEDS_H
#define _GWSPILEDS_H
#include "GwApi.h"
//task function
void handleSpiLeds(GwApi *param);

DECLARE_USERTASK(handleSpiLeds);
#endif