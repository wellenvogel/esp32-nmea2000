
//we only compile for some boards
#ifdef BOARD_TEST
#include "GwExampleTask.h"
#include "GwApi.h"
#include "GWConfig.h"
#include <vector>
#include "N2kMessages.h"
#include "GwXdrTypeMappings.h"

/**
 * INVALID!!! - the next interface declaration will not work
 *              as it is not in the correct header file
 *              it is just included here to show you how errors
 *              could be created.
 *              if you call the apiSetExampleNotWorkingIf method
 *              it will always return false
*/
class ExampleNotWorkingIf: public GwApi::TaskInterfaces::Base{
    public:
    int someValue=99;
};
DECLARE_TASKIF(ExampleNotWorkingIf);
void exampleTask(GwApi *param);
/**
 * an init function that ist being called before other initializations from the core
 */
void exampleInit(GwApi *api){
    api->getLogger()->logDebug(GwLog::LOG,"example init running");
    // make the task known to the core
    // the task function should not return (unless you delete the task - see example code)
    const String taskName("exampleTask");
    api->addUserTask(exampleTask, taskName, 4000);
    // this would create our task with a stack size of 4000 bytes

    // we declare some capabilities that we can
    // use in config.json to only show some
    // elements when this capability is set correctly
    api->addCapability("testboard", "true");
    api->addCapability("testboard2", "true");
    // hide some config value
    // and force it's default value
    auto current=api->getConfig()->getConfigItem(GwConfigDefinitions::minXdrInterval,false);
    String defaultXdrInt="50";
    if (current){
        defaultXdrInt=current->getDefault();
    }
    //with the true parameter this config value will be hidden
    //if you would like the user to be able to see this item, omit the "true", the config value will be read only
    api->getConfig()->setValue(GwConfigDefinitions::minXdrInterval,defaultXdrInt,true);
    // example for a user defined help url that will be shown when clicking the help button
    api->addCapability("HELP_URL", "https://www.wellenvogel.de");

    //we would like to store data with the interfaces that we defined
    //the name must match the one we used for addUserTask
    api->taskInterfaces()->claim<ExampleTaskIf>(taskName);
    //not working interface
    if (!api->taskInterfaces()->claim<ExampleNotWorkingIf>(taskName)){
        api->getLogger()->logDebug(GwLog::ERROR,"unable to claim ExampleNotWorkingIf");
    }
    //check if we should simulate some voltage measurements
    //add an XDR mapping in this case
    String voltageTransducer=api->getConfig()->getString(GwConfigDefinitions::exTransducer); 
    if (!voltageTransducer.isEmpty()){
        int instance=api->getConfig()->getInt(GwConfigDefinitions::exInstanceId);
        GwXDRMappingDef xdr;
        //we send a battery message - so it is category battery
        xdr.category=GwXDRCategory::XDRBAT;
        //we only need a conversion from N2K to NMEA0183
        xdr.direction=GwXDRMappingDef::Direction::M_FROM2K;
        //you can find the XDR field and selector definitions
        //in the generated GwXdrTypeMappings.h
        xdr.field=GWXDRFIELD_BATTERY_BATTERYVOLTAGE;
        //xdr.selector=-1; //refer to xdrconfig.json - there is no selector under Battery, so we can leave it empty
        //we just map exactly our instance
        xdr.instanceMode=GwXDRMappingDef::IS_SINGLE; 
        
        //those are the user configured values
        //this instance is the one we use for the battery instance when we set up 
        //the N2K message
        xdr.instanceId=instance;
        xdr.xdrName=voltageTransducer;
        if (!api->addXdrMapping(xdr)){
            api->getLogger()->logDebug(GwLog::ERROR,"unable to set our xdr mapping %s",xdr.toString().c_str());
        }
    }
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
            latitude=api->getBoatData()->LAT->getDataWithDefault(INVALID_COORD);
            longitude=api->getBoatData()->LON->getDataWithDefault(INVALID_COORD);
        };
};

String formatValue(GwApi::BoatValue *value){
    if (! value->valid) return "----";
    static const int bsize=30;
    char buffer[bsize+1];
    buffer[0]=0;
    if (value->getFormat() == GwBoatItemBase::formatDate){
        time_t tv=tNMEA0183Msg::daysToTime_t(value->value);
        tmElements_t parts;
        tNMEA0183Msg::breakTime(tv,parts);
        snprintf(buffer,bsize,"%04d/%02d/%02d",parts.tm_year+1900,parts.tm_mon+1,parts.tm_mday);
    }
    else if(value->getFormat() == GwBoatItemBase::formatTime){
        double inthr;
        double intmin;
        double intsec;
        double val;
        val=modf(value->value/3600.0,&inthr);
        val=modf(val*3600.0/60.0,&intmin);
        modf(val*60.0,&intsec);
        snprintf(buffer,bsize,"%02.0f:%02.0f:%02.0f",inthr,intmin,intsec);
    }
    else if (value->getFormat() == GwBoatItemBase::formatFixed0){
        snprintf(buffer,bsize,"%.0f",value->value);
    }
    else{
        snprintf(buffer,bsize,"%.4f",value->value);
    }
    buffer[bsize]=0;
    return String(buffer);
}

class ExampleWebData{
    SemaphoreHandle_t lock;
    int data=0;
    public:
    ExampleWebData(){
        lock=xSemaphoreCreateMutex();
    }
    ~ExampleWebData(){
        vSemaphoreDelete(lock);
    }
    void set(int v){
        GWSYNCHRONIZED(lock);
        data=v;
    }
    int get(){
        GWSYNCHRONIZED(lock);
        return data;
    }
};

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
    LOG_DEBUG(GwLog::LOG,"example switch is %s",exampleSwitch?"true":"false");
    LOG_DEBUG(GwLog::LOG,"minXdrInterval=%d",api->getConfig()->getInt(api->getConfig()->minXdrInterval));
    GwApi::BoatValue *longitude=new GwApi::BoatValue(GwBoatData::_LON);
    GwApi::BoatValue *latitude=new GwApi::BoatValue(GwBoatData::_LAT);
    GwApi::BoatValue *testValue=new GwApi::BoatValue(boatItemName);
    GwApi::BoatValue *valueList[]={longitude,latitude,testValue};
    GwApi::Status status;
    int counter=api->addCounter("usertest");
    int apiResult=0;
    ExampleTaskIf e1=api->taskInterfaces()->get<ExampleTaskIf>(apiResult);
    LOG_DEBUG(GwLog::LOG,"exampleIf before rs=%d,v=%d,s=%s",apiResult,e1.count,e1.someValue.c_str());
    ExampleNotWorkingIf nw1;
    bool nwrs=api->taskInterfaces()->set(nw1);
    LOG_DEBUG(GwLog::LOG,"exampleNotWorking update returned %d",(int)nwrs);
    String voltageTransducer=api->getConfig()->getString(GwConfigDefinitions::exTransducer);
    int voltageInstance=api->getConfig()->getInt(GwConfigDefinitions::exInstanceId);
    ExampleWebData webData;
    /**
     * an example web request handler
     * it uses a synchronized data structure as it gets called from a different thread
     * be aware that you must not block for longer times here!
    */
    api->registerRequestHandler("data",[&webData](AsyncWebServerRequest *request){
        int data=webData.get();
        char buffer[30];
        snprintf(buffer,29,"%d",data);
        buffer[29]=0;
        request->send(200,"text/plain",buffer); 
    });
    int loopcounter=0;
    while(true){
        delay(1000);
        loopcounter++;
        webData.set(loopcounter);
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
        //increment some counter
        api->increment(counter,"Test");       
        ExampleTaskIf e2=api->taskInterfaces()->get<ExampleTaskIf>(apiResult);
        LOG_DEBUG(GwLog::LOG,"exampleIf before update rs=%d,v=%d,s=%s",apiResult,e2.count,e2.someValue.c_str());
        e1.count+=1;
        e1.someValue="running";
        bool rs=api->taskInterfaces()->set(e1);
        LOG_DEBUG(GwLog::LOG,"exampleIf update rs=%d,v=%d,s=%s",(int)rs,e1.count,e1.someValue.c_str());
        ExampleTaskIf e3=api->taskInterfaces()->get<ExampleTaskIf>(apiResult);
        LOG_DEBUG(GwLog::LOG,"exampleIf after update rs=%d,v=%d,s=%s",apiResult,e3.count,e3.someValue.c_str());
        if (!voltageTransducer.isEmpty()){
            //simulate some voltage measurements...
            double offset=100.0*(double)std::rand()/RAND_MAX - 50.0;
            double simVoltage=(1200.0+offset)/100;
            LOG_DEBUG(GwLog::LOG,"simulated voltage %f",(float)simVoltage);
            tN2kMsg msg;
            SetN2kDCBatStatus(msg,voltageInstance,simVoltage);
            //we send out an N2K message
            //and as we added an XDR mapping, we will see this in the data dashboard
            //and on the NMEA0183 stream
            api->sendN2kMessage(msg);
        }
    }
    vTaskDelete(NULL);
    
}

#endif