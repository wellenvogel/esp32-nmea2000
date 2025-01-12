#ifndef _GWSPILEDS_H
#define _GWSPILEDS_H
#include "GwSynchronized.h"
#include "GwApi.h"
#include "OBP60Hardware.h"

class Color{
    public:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    Color():r(0),g(0),b(0){}
    Color(uint8_t cr, uint8_t cg,uint8_t cb):
        b(cb),g(cg),r(cr){}
    Color(const Color &o):b(o.b),g(o.g),r(o.r){}
    bool equal(const Color &o) const{
        return o.r == r && o.g == g && o.b == b;
    }
    bool operator == (const Color &other) const{
        return equal(other);
    }
    bool operator != (const Color &other) const{
        return ! equal(other);
    }
};

static Color COLOR_GREEN=Color(0,255,0);
static Color COLOR_RED=Color(255,0,0);
static Color COLOR_BLUE=Color(0,0,255);
static Color COLOR_WHITE=Color(255,255,255);
static Color COLOR_BLACK=Color(0,0,0);

Color setBrightness(const Color &color,uint8_t brightness);


class LedInterface {
    private:
    bool equals(const Color *v1, const Color *v2, int num) const{
        for (int i=0;i<num;i++){
            if (!v1->equal(*v2)) return false;
            v1++;
            v2++;
        }
        return true;
    }
    void set(Color *v,int len, const Color &c){
        for (int i=0;i<len;i++){
            *v=c;
            v++;
        }
    }
    public:
    Color flash[NUM_FLASH_LED];
    Color backlight[NUM_BACKLIGHT_LED];
    int flashLen() const {return NUM_FLASH_LED;}
    int backlightLen() const {return NUM_BACKLIGHT_LED;}
    bool flasChanged(const LedInterface &other){
        return ! equals(flash,other.flash,flashLen());
    }
    bool backlightChanged(const LedInterface &other){
        return ! equals(backlight,other.backlight,backlightLen());
    }
    void setFlash(const Color &c){
        set(flash,flashLen(),c);
    }
    void setBacklight(const Color &c){
        set(backlight,backlightLen(),c);
    }
};
class LedTaskData{
    private:
        SemaphoreHandle_t locker;
        LedInterface leds;
        long updateCount=0;
    public:
        GwApi *api=NULL;
        LedTaskData(GwApi *api){
            locker=xSemaphoreCreateMutex();
            this->api=api;
        }
        void setLedData(const LedInterface &values){
            GWSYNCHRONIZED(&locker);
            leds=values;
        }
        LedInterface getLedData(){
            GWSYNCHRONIZED(&locker);
            return leds;
        }
};

//task function
void createSpiLedTask(LedTaskData *param);

#endif