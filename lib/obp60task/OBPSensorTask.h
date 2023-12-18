#pragma once
#include "GwSynchronized.h"
#include "GwApi.h"
#include "freertos/semphr.h"
#include "Pagedata.h"

class SharedData{
    private:
        SemaphoreHandle_t locker;
        SensorData sensors;
    public:
        GwApi *api=NULL;
        SharedData(GwApi *api){
            locker=xSemaphoreCreateMutex();
            this->api=api;
        }
        void setSensorData(SensorData &values){
            GWSYNCHRONIZED(&locker);
            sensors=values;
        }
        SensorData getSensorData(){
            GWSYNCHRONIZED(&locker);
            return sensors;
        }
};

void createSensorTask(SharedData *shared);


