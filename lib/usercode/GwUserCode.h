#ifndef _GWUSERCODE_H
#define _GWUSERCODE_H
#include <Arduino.h>
#include <map>
class GwLog;
class GwApi;
class GwUserCode{
    GwLog *logger;
    GwApi *api;
    public:
        typedef std::map<String,String> Capabilities;
        GwUserCode(GwApi *api);
        void startUserTasks(int baseId);
        void startAddonTask(String name,TaskFunction_t task, int id);
        Capabilities *getCapabilities();
};
#endif