#include "GwButtons.h"
#include "GwHardware.h"
#include "GwApi.h"
#include "GwLeds.h"

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
void handleButtons(void *param){
    GwApi *api=(GwApi*)param;
    GwLog *logger=api->getLogger();
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
    const unsigned long HARD_REST_TIME=10000;
    GwLedMode ledMode=LED_OFF;
    while(true){
        delay(10);
        int current=digitalRead(GWBUTTON_PIN);
        unsigned long now=millis();
        if (current != activeState){
            if (lastPressed != 0 && (lastPressed+OFF_TIME) < now){
                lastPressed=0; //finally off
                firstPressed=0;
                if (ledMode != LED_OFF){
                    setLedMode(LED_GREEN); //TODO: better "go back"
                    ledMode=LED_OFF;
                }
                LOG_DEBUG(GwLog::LOG,"Button press stopped");
            }
            continue;
        }
        lastPressed=now;
        if (firstPressed == 0) {
            firstPressed=now;
            LOG_DEBUG(GwLog::LOG,"Button press started");
            lastReport=now;
        }
        if (lastReport != 0 && (lastReport + REPORT_TIME) < now ){
            LOG_DEBUG(GwLog::LOG,"Button active for %ld",(now-firstPressed));
            lastReport=now;
        }
        GwLedMode nextMode=ledMode;
        if (now > (firstPressed+HARD_REST_TIME/2)){
            nextMode=LED_BLUE;
        }
        if (now > (firstPressed+HARD_REST_TIME*0.9)){
            nextMode=LED_RED;
        }
        if (ledMode != nextMode){
            setLedMode(nextMode);
            ledMode=nextMode;
        }
        if (now > (firstPressed+HARD_REST_TIME)){
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