#include "GwUserCode.h"
#include <Arduino.h>
#include <vector>
#include <map>
//user task handling
class UserTask{
    public:
        String name;
        TaskFunction_t task;
        UserTask(String name,TaskFunction_t task){
            this->name=name;
            this->task=task;
        }
};

std::vector<UserTask> userTasks;
GwUserCode::Capabilities userCapabilities;

void registerUserTask(TaskFunction_t task,String name){
  userTasks.push_back(UserTask(name,task));
}

class GwUserTask{
    public:
        GwUserTask(TaskFunction_t task,String name){
            registerUserTask(task,name);
        }
};
class GwUserCapability{
    public:
        GwUserCapability(String name,String value){
            userCapabilities[name]=value;  
        }
};
#define DECLARE_USERTASK(task) GwUserTask __##task##__(task,#task);
#define DECLARE_CAPABILITY(name,value) GwUserCapability __CAP##name__(#name,#value); 
#include "GwUserTasks.h"
#include "GwApi.h"
class TaskApi : public GwApi
{
    GwApi *api;
    int sourceId;

public:
    TaskApi(GwApi *api, int sourceId)
    {
        this->sourceId = sourceId;
        this->api = api;
    }
    virtual GwRequestQueue *getQueue()
    {
        return api->getQueue();
    }
    virtual void sendN2kMessage(const tN2kMsg &msg)
    {
        api->sendN2kMessage(msg);
    }
    virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, int sourceId)
    {
        api->sendNMEA0183Message(msg, sourceId);
    }
    virtual int getSourceId()
    {
        return sourceId;
    };
    virtual GwConfigHandler *getConfig()
    {
        return api->getConfig();
    }
    virtual GwLog *getLogger()
    {
        return api->getLogger();
    }
    virtual GwBoatData *getBoatData()
    {
        return api->getBoatData();
    }
};

GwUserCode::GwUserCode(GwApi *api){
    this->logger=api->getLogger();
    this->api=api;
}
static void startAddOnTask(GwApi *api,TaskFunction_t task,int sourceId){
  TaskApi* taskApi=new TaskApi(api,sourceId);
  xTaskCreate(task,"user",2000,taskApi,3,NULL);
}
void GwUserCode::startUserTasks(int baseId){
    LOG_DEBUG(GwLog::DEBUG,"starting %d user tasks",userTasks.size());
    for (auto it=userTasks.begin();it != userTasks.end();it++){
        LOG_DEBUG(GwLog::LOG,"starting user task %s with id %d",it->name.c_str(),baseId);
        startAddOnTask(api,it->task,baseId);
        baseId++;
    }
}
void GwUserCode::startAddonTask(String name, TaskFunction_t task, int id){
    LOG_DEBUG(GwLog::LOG,"starting addon task %s with id %d",name.c_str(),id);
    startAddOnTask(api,task,id);
}

GwUserCode::Capabilities * GwUserCode::getCapabilities(){
    return &userCapabilities;
}