#define DECLARE_USERTASK(task) GwUserTaskDef __##task##__(task,#task);
#define DECLARE_USERTASK_PARAM(task,...) GwUserTaskDef __##task##__(task,#task,__VA_ARGS__);
#define DECLARE_INITFUNCTION(task) GwInitTask __Init##task##__(task,#task);
#define DECLARE_CAPABILITY(name,value) GwUserCapability __CAP##name##__(#name,#value);
#define DECLARE_STRING_CAPABILITY(name,value) GwUserCapability __CAP##name##__(#name,value); 

#include "GwUserCode.h"
#include "GwSynchronized.h"
#include <Arduino.h>
#include <vector>
#include <map>
#include "GwCounter.h"
//user task handling



std::vector<GwUserTask> userTasks;
std::vector<GwUserTask> initTasks;
GwUserCode::Capabilities userCapabilities;
class RegEntry{
    public:
    String file;
    String task;
    RegEntry(const String &t, const String &f):file(f),task(t){}
    RegEntry(){}
};
using RegMap=std::map<String,RegEntry>;
static RegMap &registrations(){
    static RegMap *regMap=new RegMap();
    return *regMap;
} 

static void registerInterface(const String &task,const String &file, const String &name){
    auto it=registrations().find(name);
    if (it != registrations().end()){
        if (it->second.file != file){
            ESP_LOGE("Assert","type %s redefined in %s original in %s",name,file,it->second.file);
            std::abort(); 
        };
        if (it->second.task != task){
            ESP_LOGE("Assert","type %s registered for multiple tasks %s and %s",name,task,it->second.task);
            std::abort(); 
        };
    }   
    else{
        registrations()[name]=RegEntry(task,file);
    }
}

class GwIreg{
    public:
        GwIreg(const String &task,const String &file, const String &name){
            registerInterface(task,file,name);
        }
    };

#define DECLARE_TASKIF(task,type) \
    DECLARE_TASKIF_IMPL(task,type) \
    GwIreg __register##type(#task,__FILE__,#type)


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
#include "GwUserTasks.h"
#include "GwUserTasksIf.h"

class TaskDataEntry{
        public:
        GwApi::TaskInterfaces::Ptr ptr;
        int updates=0;
        TaskDataEntry(GwApi::TaskInterfaces::Ptr p):ptr(p){}
        TaskDataEntry(){}
    };
class TaskInterfacesStorage{
   GwLog *logger;
    SemaphoreHandle_t lock;
    std::map<String,TaskDataEntry> values;
    public:
        TaskInterfacesStorage(GwLog* l):
            logger(l){
                lock=xSemaphoreCreateMutex();
            }
        bool set(const String &file, const String &name, const String &task,GwApi::TaskInterfaces::Ptr v){
            GWSYNCHRONIZED(&lock);
            auto it=registrations().find(name);
            if (it == registrations().end()){
                LOG_DEBUG(GwLog::ERROR,"invalid set %s not known",name.c_str());
                return false;
            }
            if (it->second.file != file){
                LOG_DEBUG(GwLog::ERROR,"invalid set %s wrong file, expected %s , got %s",name.c_str(),it->second.file.c_str(),file.c_str());
                return false;
            }
            if (it->second.task != task){
                LOG_DEBUG(GwLog::ERROR,"invalid set %s wrong task, expected %s , got %s",name.c_str(),it->second.task.c_str(),task.c_str());
                return false;
            }
            auto vit=values.find(name);
            if (vit != values.end()){
                vit->second.updates++;
                vit->second.ptr=v;
            }
            else{
                values[name]=TaskDataEntry(v);
            }
            return true;
        }
        GwApi::TaskInterfaces::Ptr get(const String &name, int &result){
            GWSYNCHRONIZED(&lock);
            auto it = values.find(name);
            if (it == values.end())
            {
                result = -1;
                return GwApi::TaskInterfaces::Ptr();
            }
            result = it->second.updates;
            return it->second.ptr;
        } 
};    
class TaskInterfacesImpl : public GwApi::TaskInterfaces{
    String task;
    TaskInterfacesStorage *storage;
    public:
        TaskInterfacesImpl(const String &n,TaskInterfacesStorage *s):
            task(n),storage(s){}
        virtual bool iset(const String &file, const String &name, Ptr v){
            return storage->set(file,name,task,v);
        }
        virtual Ptr iget(const String &name, int &result){
            return storage->get(name,result);
        }
};


class TaskApi : public GwApiInternal
{
    GwApiInternal *api=nullptr;
    int sourceId;
    SemaphoreHandle_t *mainLock;
    SemaphoreHandle_t localLock;
    std::map<int,GwCounter<String>> counter;
    String name;
    bool counterUsed=false;
    int counterIdx=0;
    TaskInterfacesImpl *interfaces;
public:
    TaskApi(GwApiInternal *api, 
        int sourceId, 
        SemaphoreHandle_t *mainLock, 
        const String &name,
        TaskInterfacesStorage *s)
    {
        this->sourceId = sourceId;
        this->api = api;
        this->mainLock=mainLock;
        this->name=name;
        localLock=xSemaphoreCreateMutex();
        interfaces=new TaskInterfacesImpl(name,s);
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
    virtual void getStatus(Status &status){
        GWSYNCHRONIZED(mainLock);
        api->getStatus(status);
    }
    virtual ~TaskApi(){
        delete interfaces;
        vSemaphoreDelete(localLock);
    };
    virtual void fillStatus(GwJsonDocument &status){
        GWSYNCHRONIZED(&localLock);
        if (! counterUsed) return;
        for (auto it=counter.begin();it != counter.end();it++){
            it->second.toJson(status);
        }
    };
    virtual int getJsonSize(){
        GWSYNCHRONIZED(&localLock);
        if (! counterUsed) return 0;
        int rt=0;
        for (auto it=counter.begin();it != counter.end();it++){
            rt+=it->second.getJsonSize();
        }
        return rt;
    };
    virtual void increment(int idx,const String &name,bool failed=false){
        GWSYNCHRONIZED(&localLock);
        counterUsed=true;
        auto it=counter.find(idx);
        if (it == counter.end()) return;
        if (failed) it->second.addFail(name);
        else (it->second.add(name));
    };
    virtual void reset(int idx){
        GWSYNCHRONIZED(&localLock);
        counterUsed=true;
        auto it=counter.find(idx);
        if (it == counter.end()) return;
        it->second.reset();
    };
    virtual void remove(int idx){
        GWSYNCHRONIZED(&localLock);
        counter.erase(idx);
    }
    virtual int addCounter(const String &name){
        GWSYNCHRONIZED(&localLock);
        counterUsed=true;
        counterIdx++;
        //avoid the need for an empty counter constructor
        auto it=counter.find(counterIdx);
        if (it == counter.end()){
            counter.insert(std::make_pair(counterIdx,GwCounter<String>("count"+name)));
        }
        else it->second=GwCounter<String>("count"+name);
        return counterIdx;
    }
    virtual TaskInterfaces * taskInterfaces(){
        return interfaces;
    }

};

GwUserCode::GwUserCode(GwApiInternal *api,SemaphoreHandle_t *mainLock){
    this->logger=api->getLogger();
    this->api=api;
    this->mainLock=mainLock;
    this->taskData=new TaskInterfacesStorage(this->logger);
}
GwUserCode::~GwUserCode(){
    delete taskData;
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
void GwUserCode::startAddOnTask(GwApiInternal *api,GwUserTask *task,int sourceId,String name){
    task->api=new TaskApi(api,sourceId,mainLock,name,taskData);
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
        it->api=new TaskApi(api,baseId,mainLock,it->name,taskData);
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

void GwUserCode::fillStatus(GwJsonDocument &status){
    for (auto it=userTasks.begin();it != userTasks.end();it++){
        if (it->api){
            it->api->fillStatus(status);
        }
    }
}
int GwUserCode::getJsonSize(){
    int rt=0;
    for (auto it=userTasks.begin();it != userTasks.end();it++){
        if (it->api){
            rt+=it->api->getJsonSize();
        }
    }
    return rt;
}