#include "GwLedTask.h"
#include "GwHardware.h"
#include "GwApi.h"
#include "FastLED.h"
typedef enum {
    LED_OFF,
    LED_GREEN,
    LED_BLUE,
    LED_RED,
    LED_WHITE
} GwLedMode;

static CRGB::HTMLColorCode colorFromMode(GwLedMode cmode){
    switch(cmode){
        case LED_BLUE:
            return CRGB::Blue;    
        case LED_GREEN:
            return CRGB::Green;
        case LED_RED:
            return CRGB::Red;
        case LED_WHITE:
            return CRGB::White;        
        default:
            return CRGB::Black;    
    }
}
void handleLeds(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef GWLED_FASTLED
        LOG_DEBUG(GwLog::LOG,"currently only fastled handling");
        delay(50);
        vTaskDelete(NULL);
        return;
    #else
    CRGB leds[1];
    #ifdef GWLED_SCHEMA
    FastLED.addLeds<GWLED_TYPE,GWLED_PIN,(EOrder)GWLED_SCHEMA>(leds,1);
    #else
    FastLED.addLeds<GWLED_TYPE,GWLED_PIN>(leds,1);
    #endif
    uint8_t brightness=api->getConfig()->getInt(GwConfigDefinitions::ledBrightness,128);
    GwLedMode currentMode=LED_GREEN;
    leds[0]=colorFromMode(currentMode);
    FastLED.setBrightness(brightness);
    FastLED.show();
    LOG_DEBUG(GwLog::LOG,"led task started with mode %d",(int)currentMode);
    int apiResult=0;
    while (true)
    {
        delay(50);
        GwLedMode newMode = currentMode;
        IButtonTask buttonState = api->taskInterfaces()->get<IButtonTask>(apiResult);
        if (apiResult >= 0)
        {
            switch (buttonState.state)
            {
            case IButtonTask::PRESSED_5:
                newMode = LED_BLUE;
                break;
            case IButtonTask::PRESSED_10:
                newMode = LED_RED;
                break;
            default:
                newMode = LED_GREEN;
                break;
            }
        }
        else
        {
            newMode = LED_WHITE;
        }
        if (newMode != currentMode)
        {
            leds[0] = colorFromMode(newMode);
            FastLED.show();
            currentMode = newMode;
        }
    }
    vTaskDelete(NULL);
    #endif
}