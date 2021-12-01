#ifndef _GWUSERCODE_H
#define _GWUSERCODE_H
#include <Arduino.h>
#include <map>
class GwLog;
class GwApi;
typedef void (*GwUserTaskFunction)(GwApi *);
class GwUserTask{
    public:
        String name;
        TaskFunction_t task=NULL;
        GwUserTaskFunction usertask=NULL;
        bool isUserTask=false;
        GwApi *api=NULL;
        GwUserTask(String name,TaskFunction_t task){
            this->name=name;
            this->task=task;
        }
        GwUserTask(String name, GwUserTaskFunction task){
            this->name=name;
            this->usertask=task;
            this->isUserTask=true;
        }
};
class GwUserCode{
    GwLog *logger;
    GwApi *api;
    SemaphoreHandle_t *mainLock;
    void startAddOnTask(GwApi *api,GwUserTask *task,int sourceId,String name);
    public:
        typedef std::map<String,String> Capabilities;
        GwUserCode(GwApi *api, SemaphoreHandle_t *mainLock);
        void startUserTasks(int baseId);
        void startInitTasks(int baseId);
        void startAddonTask(String name,TaskFunction_t task, int id);
        Capabilities *getCapabilities();
};
#endif