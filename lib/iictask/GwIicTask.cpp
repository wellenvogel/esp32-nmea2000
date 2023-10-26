//#ifdef _GWIIC
#include "GwIicTask.h"
#include "GwHardware.h"
#include <Wire.h>
void runIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        LOG_DEBUG(GwLog::LOG,"no iic defined, iic task stopped");
        vTaskDelete(NULL);
        return;
    #endif
    #ifndef GWIIC_SDA
        #define GWIIC_SDA -1
    #endif
    #ifndef GWIIC_SCL
        #define GWIIC_SCL -1
    #endif
    LOG_DEBUG(GwLog::LOG,"iic task started");
    bool rt=Wire.begin(GWIIC_SDA,GWIIC_SCL);
    if (! rt){
        LOG_DEBUG(GwLog::ERROR,"unable to initialize IIC");
        vTaskDelete(NULL);
        return;
    }
    vTaskDelete(NULL);
}
//#endif
