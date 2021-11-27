#ifndef _GWUSERCODE_H
#define _GWUSERCODE_H
#include <Arduino.h>
class GwLog;
class GwApi;
class GwUserCode{
    GwLog *logger;
    GwApi *api;
    public:
        GwUserCode(GwApi *api);
        void startUserTasks(int baseId);
        void startAddonTask(String name,TaskFunction_t task, int id);
};
#endif