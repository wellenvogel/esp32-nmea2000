#include "GwUserCode.h"
#include "GwSynchronized.h"
#include <Arduino.h>
#include <vector>
#include <map>
//user task handling



std::vector<GwUserTask> userTasks;
std::vector<GwUserTask> initTasks;
GwUserCode::Capabilities userCapabilities;



class GwUserTaskDef{
    public:
        GwUserTaskDef(TaskFunction_t task,String name, int stackSize=2000){
            userTasks.push_back(GwUserTask(name,task,stackSize));
        }
        GwUserTaskDef(GwUserTaskFunction task,String name,int stackSize=2000){
            userTasks.push_back(GwUserTask(name,task,stackSize));
        }
};

class GwInitTask{
    public:
        GwInitTask(TaskFunction_t task, String name){
            initTasks.push_back(GwUserTask(name,task));
        }
        GwInitTask(GwUserTaskFunction task, String name){
            initTasks.push_back(GwUserTask(name,task));
        }
};
class GwUserCapability{
    public:
        GwUserCapability(String name,String value){
            userCapabilities[name]=value;  
        }
};
#define DECLARE_USERTASK(task) GwUserTaskDef __##task##__(task,#task);
#define DECLARE_USERTASK_PARAM(task,...) GwUserTaskDef __##task##__(task,#task,__VA_ARGS__);
#define DECLARE_INITFUNCTION(task) GwInitTask __Init##task##__(task,#task);
#define DECLARE_CAPABILITY(name,value) GwUserCapability __CAP##name##__(#name,#value); 
#include "GwApi.h"
#include "GwUserTasks.h"
class GwUserApiImpl : public GwApi
{
    GwApi *api;
    int sourceId;
    SemaphoreHandle_t *mainLock;
    bool receive0183=false;
    bool include0183Converted=false;
    QueueHandle_t nmea0183Queue;


public:
    GwUserApiImpl(GwApi *api, int sourceId, SemaphoreHandle_t *mainLock)
    {
        this->sourceId = sourceId;
        this->api = api;
        this->mainLock=mainLock;
    }
    virtual GwRequestQueue *getQueue()
    {
        return api->getQueue();
    }
    virtual void sendN2kMessage(const tN2kMsg &msg,bool convert)
    {
        GWSYNCHRONIZED(mainLock);
        api->sendN2kMessage(msg,convert);
    }
    virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, int sourceId, bool convert)
    {
        GWSYNCHRONIZED(mainLock);
        api->sendNMEA0183Message(msg, this->sourceId,convert);
    }
    virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, bool convert)
    {
        GWSYNCHRONIZED(mainLock);
        api->sendNMEA0183Message(msg, this->sourceId,convert);
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
    virtual const char* getTalkerId(){
        return api->getTalkerId();
    }
    virtual void getBoatDataValues(int num,BoatValue **list){
        GWSYNCHRONIZED(mainLock);
        api->getBoatDataValues(num,list);
    }
    virtual void receiveNMEA0183(bool includeConverted, int queueLen = 5)
    {
        api->getLogger()->logDebug(GwLog::LOG,"enable NMEA0183 for task %d, queuelen=%d",sourceId,queueLen);
        if (!receive0183)
        {
            nmea0183Queue = xQueueCreate(queueLen, sizeof(char *));
            receive0183 = true;
        }
        include0183Converted = includeConverted;
    }
    virtual bool getNMEA0183Message(tNMEA0183Msg &msg, unsigned long waitTime = 0)
    {
        if (!receive0183)
            return false;
        char *rmsg = NULL;
        bool rt = xQueueReceive(nmea0183Queue, &rmsg, pdMS_TO_TICKS(waitTime));
        if (rt)
            msg.SetMessage(rmsg);
        delete rmsg;
        return rt;
    }
    bool handleNMEA0183(const char *buf, bool fromConverter = false)
    {
        if (!receive0183)
            return false;
        if (fromConverter && !include0183Converted)
            return false;
        size_t len = strlen(buf);
        char *m = new char[len + 1];
        memcpy(m, buf, len + 1);
        if (!xQueueSend(nmea0183Queue, &m, 0))
        {
            delete m;
            return false;
        }
        return true;
    }
    virtual ~GwUserApiImpl(){};
};

GwUserCode::GwUserCode(GwApi *api,SemaphoreHandle_t *mainLock){
    this->logger=api->getLogger();
    this->api=api;
    this->mainLock=mainLock;
}
void userTaskStart(void *p){
    GwUserTask *task=(GwUserTask*)p;
    if (task->isUserTask){
        task->usertask(task->api);
    }
    else{
        task->task(task->api);
    }
    delete task->api;
    task->api=NULL;
}
void GwUserCode::startAddOnTask(GwApi *api,GwUserTask *task,int sourceId,String name){
    GwUserApiImpl *apiImpl=new GwUserApiImpl(api,sourceId,mainLock);
    apis.push_back(apiImpl);
    task->api=apiImpl;
    xTaskCreate(userTaskStart,name.c_str(),task->stackSize,task,3,NULL);
}
void GwUserCode::startUserTasks(int baseId){
    LOG_DEBUG(GwLog::DEBUG,"starting %d user tasks",userTasks.size());
    for (auto it=userTasks.begin();it != userTasks.end();it++){
        LOG_DEBUG(GwLog::LOG,"starting user task %s with id %d, stackSize %d",it->name.c_str(), baseId,it->stackSize);
        startAddOnTask(api,&(*it),baseId,it->name);
        baseId++;
    }
}
void GwUserCode::startInitTasks(int baseId){
    LOG_DEBUG(GwLog::DEBUG,"starting %d user init tasks",initTasks.size());
    for (auto it=initTasks.begin();it != initTasks.end();it++){
        LOG_DEBUG(GwLog::LOG,"starting user init task %s with id %d",it->name.c_str(),baseId);
        GwUserApiImpl *apiImpl=new GwUserApiImpl(api,baseId,mainLock);
        it->api=apiImpl;
        userTaskStart(&(*it));
        baseId++;
    }
}
void GwUserCode::startAddonTask(String name, TaskFunction_t task, int id){
    LOG_DEBUG(GwLog::LOG,"starting addon task %s with id %d",name.c_str(),id);
    GwUserTask *userTask=new GwUserTask(name,task); //memory leak - acceptable as only during startup
    startAddOnTask(api,userTask,id,userTask->name);
}

GwUserCode::Capabilities * GwUserCode::getCapabilities(){
    return &userCapabilities;
}
bool GwUserCode::handleNMEA0183(const char *buffer,bool fromConverter){
    bool rt=false;
    for (auto it=apis.begin();it != apis.end();it++){
        if((*it)->handleNMEA0183(buffer,fromConverter)) rt=true;
    }
    return rt;
}