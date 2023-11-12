#include "SerTestTask.h"
#include "GwHardware.h"
#ifdef SERTEST
void sertest(GwApi *api){
    GwLog *logger=api->getLogger();
    LOG_DEBUG(GwLog::LOG,"ser test task started");
    Serial2.begin(9600,SERIAL_8N1,-1,GWSERIAL_RX);
    while (true){
        const char * T="SERTESTXXX";
        LOG_DEBUG(GwLog::LOG,"sending %s",T);
        Serial2.println(T);
        delay(1000);
    }
    vTaskDelete(NULL);
    return;
}

void sertestInit(GwApi *api){
    GwLog *logger=api->getLogger();
    LOG_DEBUG(GwLog::LOG,"ser test init");
    api->addUserTask(sertest,"SERTEST",4000);
}
#endif