#pragma once
#include <freertos/semphr.h>

class GwSynchronized{
    private:
        SemaphoreHandle_t locker=nullptr;
        void lock(){
            if (locker != nullptr) xSemaphoreTake(locker, portMAX_DELAY);
        }
    public:
        /**
         * deprecated
         * as SemaphoreHandle_t is already a pointer just use this directly
        */
        GwSynchronized(SemaphoreHandle_t *locker){
            if (locker == nullptr) return;
            this->locker=*locker;
            lock();
        }
        GwSynchronized(SemaphoreHandle_t locker){
            this->locker=locker;
            lock();
        }
        ~GwSynchronized(){
            if (locker != nullptr) xSemaphoreGive(locker);
        }
};

#define GWSYNCHRONIZED(locker) GwSynchronized __xlock__(locker);