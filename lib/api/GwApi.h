#ifndef _GWAPI_H
#define _GWAPI_H
#include "GwMessage.h"
#include "N2kMsg.h"
#include "NMEA0183Msg.h"
#include "GWConfig.h"
#include "GwBoatData.h"
#include <map>
#include <vector>
//API to be used for additional tasks
class GwApi{
    public:
        typedef std::map<String,double> ValueMap;
        typedef std::vector<String> StringList;
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
         * get a set of boat data values by their names
         * the returned map will have the keys being the strings in the names list
         * the values are the boat data values converted to double
         * invalid values are not returned in the map
         * this method is thread safe and can directly be used from a user task
         */
        virtual ValueMap getBoatDataValues(StringList)=0;
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
#endif
