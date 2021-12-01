#pragma once
#include <freertos/semphr.h>

class GwSynchronized{
    private:
        SemaphoreHandle_t *locker;
    public:
        GwSynchronized(SemaphoreHandle_t *locker){
            this->locker=locker;
            xSemaphoreTake(*locker, portMAX_DELAY);
        }
        ~GwSynchronized(){
            xSemaphoreGive(*locker);
        }
};

#define GWSYNCHRONIZED(locker) GwSynchronized __xlock__(locker);