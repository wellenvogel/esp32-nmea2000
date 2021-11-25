
#include "GwApi.h"

#define INVALID_COORD -99999
class GetBoatDataRequest: public GwMessage{
    private:
        GwApi *api;
    public:
        double latitude;
        double longitude;
        GetBoatDataRequest(GwApi *api):GwMessage(F("boat data")){
            this->api=api;
        }
        virtual ~GetBoatDataRequest(){}
    protected:
        /**
         * this methos will be executed within the main thread
         * be sure not to make any time consuming or blocking operation
         */    
        virtual void processImpl(){
            //api->getLogger()->logDebug(GwLog::DEBUG,"boatData request from example task");
            /*access the values from boatData (see GwBoatData.h)
              by using getDataWithDefault it will return the given default value
              if there is no valid data available
              so be sure to use a value that never will be a valid one
              alternatively you can check using the isValid() method at each boatData item
              */
            latitude=api->getBoatData()->Latitude->getDataWithDefault(INVALID_COORD);
            longitude=api->getBoatData()->Longitude->getDataWithDefault(INVALID_COORD);
        };
};
void exampleTask(void *param){
    GwApi *api=(GwApi*)param;
    GwLog *logger=api->getLogger();
    //------
    //initialization goes here
    //------
    bool hasPosition=false;
    while(true){
        delay(1000);
        /*
        * getting values from the internal data store (boatData) requires some special handling
        * our tasks runs (potentially) at some time on a different core then the main code
        * and as the boatData has no synchronization (for performance reasons)
        * we must ensure to access it only from the main thread.
        * The pattern is to create a request object (a class that inherits from GwMessage -
        * GetBoatDataRequest above)
        * and to access the boatData in the processImpl method of this object.
        * Once the object is created we enqueue it into a request queue and wait for
        * the main thread to call the processImpl method (sendAndWait).
        * Afterwards we can use the data we have stored in the request.
        * As this request object can be accessed from different threads we must be careful
        * about it's lifetime.
        * The pattern below handles this correctly. We do not call delete on this object but
        * instead call the "unref" Method when we don't need it any more.
        */
        GetBoatDataRequest *r=new GetBoatDataRequest(api);
        if (api->getQueue()->sendAndWait(r,10000) != GwRequestQueue::MSG_OK){
            r->unref(); //delete the request
            api->getLogger()->logDebug(GwLog::ERROR,"request not handled");
            continue;
        }
        if (r->latitude == INVALID_COORD || r->longitude == INVALID_COORD){
            if (hasPosition){
                logger->logDebug(GwLog::ERROR,"position lost...");
                hasPosition=false;
            }
        }
        else{
            //do something with the data we have from boatData
            if (! hasPosition){
                logger->logDebug(GwLog::LOG,"postion now available lat=%f, lon=%f",
                    r->latitude,r->longitude);
                    hasPosition=true;
            }

        }
        r->unref(); //delete the request
    }
    vTaskDelete(NULL);
    
}