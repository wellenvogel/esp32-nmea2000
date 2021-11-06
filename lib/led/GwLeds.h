#ifndef _GWLEDS_H
#define _GWLEDS_H
//task function
void handleLeds(void *param);
typedef enum {
    LED_OFF,
    LED_GREEN,
    LED_BLUE,
    LED_RED,
    LED_WHITE
} GwLedMode;
void setLedMode(GwLedMode mode);
#endif