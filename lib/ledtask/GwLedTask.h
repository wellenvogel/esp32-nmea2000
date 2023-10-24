#ifndef _GWLEDS_H
#define _GWLEDS_H
#include "GwApi.h"
//task function
void handleLeds(GwApi *param);
typedef enum {
    LED_OFF,
    LED_GREEN,
    LED_BLUE,
    LED_RED,
    LED_WHITE
} GwLedMode;
void setLedMode(GwLedMode mode);
DECLARE_USERTASK(handleLeds);
#endif