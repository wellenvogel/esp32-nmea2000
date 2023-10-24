#include "GwButtonTask.h"
#include "GwIButtonTask.h"
#include "GwHardware.h"
#include "GwApi.h"
#include "GwLedTask.h"

class FactoryResetRequest: public GwMessage{
    private:
        GwApi *api;
    public:
        FactoryResetRequest(GwApi *api):GwMessage(F("reset button")){
            this->api=api;
        }
        virtual ~FactoryResetRequest(){}
    protected:    
        virtual void processImpl(){
            api->getLogger()->logDebug(GwLog::LOG,"reset request processing");
            api->getConfig()->reset();
            xTaskCreate([](void *p){
            delay(500);
            ESP.restart();
            vTaskDelete(NULL);
            },"reset",1000,NULL,0,NULL);
        };
};
void handleButtons(GwApi *api){
    GwLog *logger=api->getLogger();
    IButtonTask state;
    if (!apiSetIButtonTask(api,state)){
        LOG_DEBUG(GwLog::ERROR,"unable to set button state");
    } 
    #ifndef GWBUTTON_PIN
        LOG_DEBUG(GwLog::LOG,"no button pin defined, do not watch");
        vTaskDelete(NULL);
        return;
    #else
    #ifndef GWBUTTON_ACTIVE
    int activeState=0; 
    #else
    int activeState=GWBUTTON_ACTIVE;
    #endif
    #ifdef GWBUTTON_PULLUPDOWN
    bool pullUpDown=true;
    #else
    bool pullUpDown=false;
    #endif
    uint8_t mode=INPUT;
    if (pullUpDown){
        mode=activeState?INPUT_PULLDOWN:INPUT_PULLUP;
    }
    pinMode(GWBUTTON_PIN,mode);
    unsigned long lastPressed=0;
    unsigned long firstPressed=0;
    unsigned long lastReport=0;
    const unsigned long OFF_TIME=20;
    const unsigned long REPORT_TIME=1000;
    const unsigned long PRESS_5_TIME=5000;
    const unsigned long PRESS_10_TIME=10000;
    const unsigned long PRESS_RESET_TIME=12000;
    LOG_DEBUG(GwLog::LOG,"button task started");
    while(true){
        delay(10);
        int current=digitalRead(GWBUTTON_PIN);
        unsigned long now=millis();
        IButtonTask::ButtonState lastState=state.state;
        if (current != activeState){
            if (lastPressed != 0 && (lastPressed+OFF_TIME) < now){
                lastPressed=0; //finally off
                firstPressed=0;
                state.state=IButtonTask::OFF;
                LOG_DEBUG(GwLog::LOG,"Button press stopped");
            }
            if (state.state != lastState){
                apiSetIButtonTask(api,state);
            }
            continue;
        }
        lastPressed=now;
        if (firstPressed == 0) {
            firstPressed=now;
            LOG_DEBUG(GwLog::LOG,"Button press started");
            state.pressCount++;
            state.state=IButtonTask::PRESSED;
            apiSetIButtonTask(api,state);
            lastReport=now;
            continue;
        }
        if (lastReport != 0 && (lastReport + REPORT_TIME) < now ){
            LOG_DEBUG(GwLog::LOG,"Button active for %ld",(now-firstPressed));
            lastReport=now;
        }
        if (now > (firstPressed+PRESS_5_TIME)){
            state.state=IButtonTask::PRESSED_5;
        }
        if (now > (firstPressed+PRESS_10_TIME)){
            state.state=IButtonTask::PRESSED_10;
        }
        if (lastState != state.state){
            apiSetIButtonTask(api,state);
        }
        if (now > (firstPressed+PRESS_RESET_TIME)){
            LOG_DEBUG(GwLog::ERROR,"Factory reset by button");
            GwMessage *r=new FactoryResetRequest(api);
            api->getQueue()->sendAndForget(r);
            r->unref();
            firstPressed=0;
            lastPressed=0;
        }
    }
    vTaskDelete(NULL);
    #endif
}