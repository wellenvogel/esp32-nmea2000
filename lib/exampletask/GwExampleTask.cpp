
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

String formatValue(GwApi::BoatValue *value){
    if (! value->valid) return "----";
    static const int bsize=30;
    char buffer[bsize+1];
    buffer[0]=0;
    if (value->getFormat() == "formatDate"){
        time_t tv=tNMEA0183Msg::daysToTime_t(value->value);
        tmElements_t parts;
        tNMEA0183Msg::breakTime(tv,parts);
        snprintf(buffer,bsize,"%04d/%02d/%02d",parts.tm_year+1900,parts.tm_mon+1,parts.tm_mday);
    }
    else if(value->getFormat() == "formatTime"){
        double inthr;
        double intmin;
        double intsec;
        double val;
        val=modf(value->value/3600.0,&inthr);
        val=modf(val*3600.0/60.0,&intmin);
        modf(val*60.0,&intsec);
        snprintf(buffer,bsize,"%02.0f:%02.0f:%02.0f",inthr,intmin,intsec);
    }
    else if (value->getFormat() == "formatFixed0"){
        snprintf(buffer,bsize,"%.0f",value->value);
    }
    else{
        snprintf(buffer,bsize,"%.4f",value->value);
    }
    buffer[bsize]=0;
    return String(buffer);
}

void exampleTask(GwApi *api){
    GwLog *logger=api->getLogger();
    //get some configuration data
    bool exampleSwitch=api->getConfig()->getConfigItem(
        api->getConfig()->exampleConfig,
        true)->asBoolean();
    String boatItemName=api->getConfig()->getString(api->getConfig()->exampleBDSel);    
    //------
    //initialization goes here
    //------
    bool hasPosition=false;
    bool hasPosition2=false;
    LOG_DEBUG(GwLog::DEBUG,"example switch ist %s",exampleSwitch?"true":"false");
    GwApi::BoatValue *longitude=new GwApi::BoatValue(F("Longitude"));
    GwApi::BoatValue *latitude=new GwApi::BoatValue(F("Latitude"));
    GwApi::BoatValue *testValue=new GwApi::BoatValue(boatItemName);
    GwApi::BoatValue *valueList[]={longitude,latitude,testValue};
    GwApi::Status status;
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
            This does not need the request handling approach
            Finally it only makes sense to use one of the versions - either with the request
            or with the ValueMap approach.
        **/
        //fetch the current values of the items that we have in itemNames
        api->getBoatDataValues(3,valueList);
        //check if the values are valid (i.e. the values we requested have been found in boatData)
        if (longitude->valid && latitude->valid){
            //both values are there - so we have a valid position
            if (! hasPosition2){
                //access to the values via iterator->second (iterator->first would be the name)
                if (exampleSwitch) LOG_DEBUG(GwLog::LOG,"(2)position availale lat=%s, lon=%s",
                    formatValue(latitude).c_str(),formatValue(longitude).c_str());
                hasPosition2=true;
            }
        }
        else{
            if (hasPosition2){
                if (exampleSwitch) LOG_DEBUG(GwLog::LOG,"(2)position lost");
                hasPosition2=false;
            }
        }
        if (testValue->valid){
            if (testValue->changed){
                LOG_DEBUG(GwLog::LOG,"%s new value %s",testValue->getName().c_str(),formatValue(testValue).c_str());
            }
        }
        else{
            if (testValue->changed){
                LOG_DEBUG(GwLog::LOG,"%s now invalid",testValue->getName().c_str());
            }
        }
        api->getStatus(status);
        #define B(v) (v?"true":"false")
        LOG_DEBUG(GwLog::LOG,"ST1:ap=%s,wc=%s,cc=%s",
            B(status.wifiApOn),
            B(status.wifiClientOn),
            B(status.wifiClientConnected));
        LOG_DEBUG(GwLog::LOG,"ST2:sn=%s,ai=%s,ap=%s,cs=%s,ci=%s",
            status.systemName.c_str(),
            status.wifiApIp.c_str(),
            status.wifiApPass.c_str(),
            status.wifiClientSSID.c_str(),
            status.wifiClientIp.c_str());
        LOG_DEBUG(GwLog::LOG,"ST3:ur=%ld,ut=%ld,sr=%ld,st=%ld,tr=%ld,tt=%ld,cr=%ld,ct=%ld,2r=%ld,2t=%ld",
            status.usbRx,
            status.usbTx,
            status.serRx,
            status.serTx,
            status.tcpSerRx,
            status.tcpSerTx,
            status.tcpClRx,
            status.tcpClTx,
            status.n2kRx,
            status.n2kTx);        

    }
    vTaskDelete(NULL);
    
}

#endif