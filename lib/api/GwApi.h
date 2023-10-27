#ifndef _GWAPI_H
#define _GWAPI_H
#include "GwMessage.h"
#include "N2kMsg.h"
#include "NMEA0183Msg.h"
#include "GWConfig.h"
#include "GwBoatData.h"
#include "GwXDRMappings.h"
#include <map>
//API to be used for additional tasks
class GwApi{
    public:
        class BoatValue{
            const String name;
            String format;
            bool formatSet=false;
            public:
                double value=0;
                bool valid=false;
                int source=-1;
                bool changed=false; //will be set by getBoatDataValues
                BoatValue(){}
                BoatValue(const String &n):name(n){
                }
                void setFormat(const String &format){
                    if (formatSet) return;
                    this->format=format;
                    formatSet=true;
                }
                const String & getName() const{
                    return name;
                }
                const String & getFormat() const{
                    return format;
                }
        };
        /**
         * an interface for the data exchange between tasks
         * the core part will not handle this data at all but
         * this interface ensures that there is a correct locking of the
         * data to correctly handle multi threading
         * The user code should not use this intterface directly
         * but instead it should use the static functions
         * apiGetXXX(GwApi *,...)
         * apiSetXXX(GwApi *,...)
         * that will be created by the macro DECLARE_TASK_INTERFACE
         */
        class TaskInterfaces
        {
        public:
            class Base
            {
            public:
                virtual ~Base()
                {
                }
            };
            using Ptr = std::shared_ptr<Base>;
        public:
            virtual bool iset(const String &file, const String &name, Ptr v) = 0;
            virtual Ptr iget(const String &name, int &result) = 0;
        };
        class Status{
            public:
                bool wifiApOn=false;
                bool wifiClientOn=false;
                bool wifiClientConnected=false;
                String wifiApIp;
                String systemName; //is also AP SSID
                String wifiApPass;
                String wifiClientIp;
                String wifiClientSSID;
                unsigned long usbRx=0;
                unsigned long usbTx=0;
                unsigned long serRx=0;
                unsigned long serTx=0;
                unsigned long ser2Rx=0;
                unsigned long ser2Tx=0;
                unsigned long tcpSerRx=0;
                unsigned long tcpSerTx=0;
                int tcpClients=0;
                unsigned long tcpClRx=0;
                unsigned long tcpClTx=0;
                bool tcpClientConnected=false;
                unsigned long n2kRx=0;
                unsigned long n2kTx=0;
                void empty(){
                    wifiApOn=false;
                    wifiClientOn=false;
                    wifiClientConnected=false;
                    wifiApIp=String();
                    systemName=String(); //is also AP SSID
                    wifiApPass=String();
                    wifiClientIp=String();
                    wifiClientSSID=String();
                    usbRx=0;
                    usbTx=0;
                    serRx=0;
                    serTx=0;
                    ser2Rx=0;
                    ser2Tx=0;
                    tcpSerRx=0;
                    tcpSerTx=0;
                    tcpClients=0;
                    tcpClRx=0;
                    tcpClTx=0;
                    tcpClientConnected=false;
                    n2kRx=0;
                    n2kTx=0;
                }
        }; 
        /**
         * thread safe methods - can directly be called from a user task
         */
        virtual GwRequestQueue *getQueue()=0;
        virtual void sendN2kMessage(const tN2kMsg &msg, bool convert=true)=0;
        /**
         * deprecated - sourceId will be ignored
         */
        virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, int sourceId,bool convert=true)=0;
        virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, bool convert=true)=0;
        virtual int getSourceId()=0;
        /**
         * the returned config data must only be read in a user task
         * writing to it is not thread safe and could lead to crashes
         */
        virtual GwConfigHandler *getConfig()=0;
        virtual GwLog *getLogger()=0;
        virtual const char* getTalkerId()=0;
        /**
         * get a set of boat data values 
         * the provided list of BoatValue must be initialized with 
         * empty boatValues (see exampleTask source code)
         * the method will fill the valid flag, the value and the format string of
         * each boat value
         * just make sure to have the list being of appropriate size (numValues)
         */
        virtual void getBoatDataValues(int numValues,BoatValue **list)=0;

        /**
         * fill the status information
         */
        virtual void getStatus(Status &status)=0;
        /**
         * access to counters for a task
         * thread safe
         * use the value returned from addCounter for the other operations
        */
        virtual int addCounter(const String &){return -1;}
        virtual void increment(int idx,const String &name,bool failed=false){}
        virtual void reset(int idx){}
        virtual void remove(int idx){}
        virtual TaskInterfaces * taskInterfaces()=0;

        /**
         * only allowed during init methods
        */
        virtual bool addXdrMapping(const GwXDRMappingDef &)=0;

        virtual void addCapability(const String &name, const String &value)=0;

        /**
         * not thread safe methods
         * accessing boat data must only be executed from within the main thread
         * you need to use the request pattern as shown in GwExampleTask.cpp
         */
        virtual GwBoatData *getBoatData()=0;
        virtual ~GwApi(){}
};
#ifndef DECLARE_USERTASK
#define DECLARE_USERTASK(task)
#endif
#ifndef DECLARE_USERTASK_PARAM
#define DECLARE_USERTASK_PARAM(task,...)
#endif
#ifndef DECLARE_INITFUNCTION
#define DECLARE_INITFUNCTION(task)
#endif
#ifndef DECLARE_CAPABILITY
#define DECLARE_CAPABILITY(name,value)
#endif
#ifndef DECLARE_STRING_CAPABILITY
#define DECLARE_STRING_CAPABILITY(name,value)
#endif
/**
 * macro to declare an interface that a task provides to other tasks
 * this macro must be used in an File I<taskname>.h or GwI<taskname>.h
 * the first parameter must be the name of the task that should be able
 * to write this data (the same name as being used in DECLARE_TASK).
 * The second parameter must be a type (class) that inherits from 
 * GwApi::TaskInterfaces::Base.
 * This class must provide copy constructors and empty constructors.
 * The attributes should only be simple values or structs but no pointers.
 * example:
 * class TestTaskApi: public GwApi::TaskInterfaces::Base{
 *      public:
 *          int ival1=0;
 *          int ival2=99;
 *          String sval="unset";
 * };
 * DECLARE_TASKIF(testTask,TestTaskApi);
 * The macro will generate 2 static funtions:
 * 
 * bool apiSetTestTaskApi(GwApi *api, const TestTaskApi &v);
 * TestTaskApi apiGetTestTaskApi(GwApi *api, int &result);
 * 
 * The setter will return true on success.
 * It is intended to be used by the task that did declare the api.
 * The getter will set the result to -1 if no data is available, otherwise
 * it will return the update count (so you can check if there was a change 
 * compared to the last call).
 * It is intended to be used by any task.
 * Be aware that all the apis share a common namespace - so be sure to somehow
 * make your API names unique.
 * 
 * 
*/
#define DECLARE_TASKIF_IMPL(task,type) \
    static bool apiSet##type(GwApi *a,const type &v){ \
        if (! a || ! a->taskInterfaces()) return false; \
        return a->taskInterfaces()->iset(__FILE__,#type,GwApi::TaskInterfaces::Ptr(new type(v)));\
    }\
    static const type apiGet##type(GwApi *a, int &result) {\
        if (! a || ! a->taskInterfaces()) {\
            result=-1; \
            return type(); \
            }\
        auto ptr=a->taskInterfaces()->iget(#type,result); \
        if (!ptr) {\
            result=-1; \
            return type(); \
        }\
        type *tp=(type*)ptr.get(); \
        return type(*tp); \
    }
#ifndef DECLARE_TASKIF
    #define DECLARE_TASKIF(task,type) DECLARE_TASKIF_IMPL(task,type)
#endif
#endif
