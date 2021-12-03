
//we only compile for some boards
#ifdef BOARD_TEST
#include "GwExampleTask.h"
#include "GwApi.h"
#include <vector>

/**
 * an init function that ist being called before other initializations from the core
 */
void exampleInit(GwApi *api){
    api->getLogger()->logDebug(GwLog::LOG,"example init running");
}
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
void exampleTask(GwApi *api){
    GwLog *logger=api->getLogger();
    //get some configuration data
    bool exampleSwitch=api->getConfig()->getConfigItem(
        api->getConfig()->exampleConfig,
        true)->asBoolean();
    //------
    //initialization goes here
    //------
    bool hasPosition=false;
    bool hasPosition2=false;
    LOG_DEBUG(GwLog::DEBUG,"example switch ist %s",exampleSwitch?"true":"false");
    GwApi::StringList itemNames;
    itemNames.push_back(F("Latitude"));
    itemNames.push_back(F("Longitude"));
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
                if (exampleSwitch) logger->logDebug(GwLog::ERROR,"position lost...");
                hasPosition=false;
            }
        }
        else{
            //do something with the data we have from boatData
            if (! hasPosition){
                if (exampleSwitch) logger->logDebug(GwLog::LOG,"postion now available lat=%f, lon=%f",
                    r->latitude,r->longitude);
                    hasPosition=true;
            }

        }
        r->unref(); //delete the request

        /** second example with string based functions to access boatData
            uses STL vector and map - but does not need any request handling
            this uses a reasonable amount of mmeory on the stack:
            up to 2x the size of the list of names and 2x the size of the returned map
            the default stack size of 2000 will not fit.
            So be sure to use DECLARA_USERTASK_PARAM(taskFuntion,stackSize) and provide
            a reasonable stack size (e.g. 4000 in this example).
            Finally it only makes sense to use one of the versions - either with the request
            or with the ValueMap approach.
            The request access to boatData gives you more options on how to access the data
            and ueses less ressources and runtime but this one is maybe easier to understand and implement
        **/
        //fetch the current values of the items that we have in itemNames
        GwApi::ValueMap boatItems=api->getBoatDataValues(itemNames);
        //get the values out of the map
        //the returned item is a map iterator
        auto longitude=boatItems.find(itemNames[1]);
        auto latitude=boatItems.find(itemNames[0]);
        //check if the iterators are valid (i.e. the values we requested have been found in boatData)
        if (longitude != boatItems.end() && latitude != boatItems.end()){
            //both values are there - so we have a valid position
            if (! hasPosition2){
                //access to the values via iterator->second (iterator->first would be the name)
                if (exampleSwitch) LOG_DEBUG(GwLog::LOG,"(2)position availale lat=%f, lon=%f",
                    latitude->second,longitude->second);
                hasPosition2=true;
            }
        }
        else{
            if (hasPosition2){
                if (exampleSwitch) LOG_DEBUG(GwLog::LOG,"(2)position lost");
                hasPosition2=false;
            }
        }

    }
    vTaskDelete(NULL);
    
}

#endif