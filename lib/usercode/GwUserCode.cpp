#define DECLARE_USERTASK(task) GwUserTaskDef __##task##__(task,#task);
#define DECLARE_USERTASK_PARAM(task,...) GwUserTaskDef __##task##__(task,#task,__VA_ARGS__);
#define DECLARE_INITFUNCTION(task) GwInitTask __Init##task##__(task,#task);
#define DECLARE_INITFUNCTION_ORDER(task,order) GwInitTask __Init##task##__(task,#task,order);
#define DECLARE_CAPABILITY(name,value) GwUserCapability __CAP##name##__(#name,#value);
#define DECLARE_STRING_CAPABILITY(name,value) GwUserCapability __CAP##name##__(#name,value); 
#define DECLARE_TASKIF(type) \
    DECLARE_TASKIF_IMPL(type) \
    static int __taskInterface##type=0; //avoid duplicate declarations

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

template <typename V>
bool taskExists(V &list, const String &name){
    for (auto it=list.begin();it!=list.end();it++){
        if (it->name == name) return true;
    }
    return false;
}
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
        GwInitTask(GwUserTaskFunction task, String name,int order=0){
            initTasks.push_back(GwUserTask(name,task,GwUserTask::DEFAULT_STACKSIZE,order));
        }
};
class GwUserCapability{
    public:
        GwUserCapability(String name,String value){
            userCapabilities[name]=value;  
        }
};
#define _NOGWHARDWAREUT
#include "GwUserTasks.h"
#undef _NOGWHARDWAREUT

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
        bool set(const String &name, GwApi::TaskInterfaces::Ptr v){
            GWSYNCHRONIZED(lock);
            auto vit=values.find(name);
            if (vit != values.end()){
                vit->second.updates++;
                if (vit->second.updates < 0){
                    vit->second.updates=0;
                }
                vit->second.ptr=v;
            }
            else{
                values[name]=TaskDataEntry(v);
            }
            return true;
        }
        GwApi::TaskInterfaces::Ptr get(const String &name, int &result){
            GWSYNCHRONIZED(lock);
            auto it = values.find(name);
            if (it == values.end())
            {
                result = -1;
                return GwApi::TaskInterfaces::Ptr();
            }
            result = it->second.updates;
            return it->second.ptr;
        }

        bool update(const String &name, std::function<GwApi::TaskInterfaces::Ptr(GwApi::TaskInterfaces::Ptr)>f){
            GWSYNCHRONIZED(lock);
            auto vit=values.find(name);
            bool rt=false;
            int mode=0;
            if (vit == values.end()){
                mode=1;
                auto np=f(GwApi::TaskInterfaces::Ptr());
                if (np){
                    mode=11;
                    values[name]=TaskDataEntry(np);
                    rt=true;
                }
            }
            else
            {
                auto np = f(vit->second.ptr);
                mode=2;
                if (np)
                {
                    mode=22;
                    vit->second = np;
                    vit->second.updates++;
                    if (vit->second.updates < 0)
                    {
                        vit->second.updates = 0;
                    }
                    rt=true;
                }
            }
            LOG_DEBUG(GwLog::DEBUG,"TaskApi::update %s (mode %d)returns %d",name.c_str(),mode,(int)rt);
            return rt;
        } 
};    
class TaskInterfacesImpl : public GwApi::TaskInterfaces{

    TaskInterfacesStorage *storage;
    GwLog *logger;
    bool isInit=false;
    public:
        TaskInterfacesImpl(TaskInterfacesStorage *s, GwLog *l,bool i):
            storage(s),isInit(i),logger(l){}
    protected:
        virtual bool iset(const String &name, Ptr v){
            return storage->set(name,v);
        }
        virtual Ptr iget(const String &name, int &result){
            return storage->get(name,result);
        }
        virtual bool iupdate(const String &name,std::function<Ptr(Ptr v)> f){
            return storage->update(name,f);
        }
};


class TaskApi : public GwApiInternal
{
    GwApiInternal *api=nullptr;
    int sourceId;
    SemaphoreHandle_t mainLock;
    SemaphoreHandle_t localLock;
    std::map<int,GwCounter<String>> counter;
    std::map<String,GwApi::HandlerFunction> webHandlers;
    String name;
    bool counterUsed=false;
    int counterIdx=0;
    TaskInterfacesImpl *interfaces;
    bool isInit=false;
public:
    TaskApi(GwApiInternal *api, 
        int sourceId, 
        SemaphoreHandle_t mainLock, 
        const String &name,
        TaskInterfacesStorage *s,
        bool init=false)
    {
        this->sourceId = sourceId;
        this->api = api;
        this->mainLock=mainLock;
        this->name=name;
        localLock=xSemaphoreCreateMutex();
        interfaces=new TaskInterfacesImpl(s,api->getLogger(),init);
        isInit=init;
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
        GWSYNCHRONIZED(localLock);
        if (! counterUsed) return;
        for (auto it=counter.begin();it != counter.end();it++){
            it->second.toJson(status);
        }
    };
    virtual int getJsonSize(){
        GWSYNCHRONIZED(localLock);
        if (! counterUsed) return 0;
        int rt=0;
        for (auto it=counter.begin();it != counter.end();it++){
            rt+=it->second.getJsonSize();
        }
        return rt;
    };
    virtual void increment(int idx,const String &name,bool failed=false){
        GWSYNCHRONIZED(localLock);
        counterUsed=true;
        auto it=counter.find(idx);
        if (it == counter.end()) return;
        if (failed) it->second.addFail(name);
        else (it->second.add(name));
    };
    virtual void reset(int idx){
        GWSYNCHRONIZED(localLock);
        counterUsed=true;
        auto it=counter.find(idx);
        if (it == counter.end()) return;
        it->second.reset();
    };
    virtual void remove(int idx){
        GWSYNCHRONIZED(localLock);
        counter.erase(idx);
    }
    virtual int addCounter(const String &name){
        GWSYNCHRONIZED(localLock);
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
    virtual bool addXdrMapping(const GwXDRMappingDef &def){
        return api->addXdrMapping(def);
    }
    virtual void registerRequestHandler(const String &url,HandlerFunction handler){
        GWSYNCHRONIZED(localLock);
        webHandlers[url]=handler;
    }
    virtual void addCapability(const String &name, const String &value){
        if (! isInit) return;
        userCapabilities[name]=value;
    }
    virtual bool addUserTask(GwUserTaskFunction task,const String tname, int stackSize=2000){
        if (! isInit){
            api->getLogger()->logDebug(GwLog::ERROR,"trying to add a user task %s outside init",tname.c_str());
            return false;
        }
        if (taskExists(userTasks,name)){
            api->getLogger()->logDebug(GwLog::ERROR,"trying to add a user task %s that already exists",tname.c_str());
            return false;
        }
        userTasks.push_back(GwUserTask(tname,task,stackSize));
        api->getLogger()->logDebug(GwLog::LOG,"adding user task %s",tname.c_str());
        return true;
    }
    virtual void setCalibrationValue(const String &name, double value){
        api->setCalibrationValue(name,value);
    }
    virtual bool handleWebRequest(const String &url, AsyncWebServerRequest *req)
    {
        GwApi::HandlerFunction handler;
        {
            GWSYNCHRONIZED(localLock);
            auto it = webHandlers.find(url);
            if (it == webHandlers.end())
            {
                api->getLogger()->logDebug(GwLog::LOG, "no web handler task=%s url=%s", name.c_str(), url.c_str());
                return false;
            }
            handler = it->second;
        }
        if (handler)
            handler(req);
        return true;
    }
    virtual void addSensor(SensorBase *sb,bool readConfig=true) override{
        if (sb == nullptr) return;
        SensorBase::Ptr sensor(sb);
        if (readConfig) sb->readConfig(this->getConfig());
        if (! sensor->ok){
               api->getLogger()->logDebug(GwLog::ERROR,"sensor %s nok , bustype=%d",sensor->prefix.c_str(),(int)sensor->busType);
               return; 
            }
        bool rt=taskInterfaces()->update<ConfiguredSensors>( [sensor,this](ConfiguredSensors *sensors)->bool{
            api->getLogger()->logDebug(GwLog::LOG,"adding sensor %s, type=%d",sensor->prefix,(int)sensor->busType);
            sensors->sensors.add(sensor);
            return true;
        });
    }
};

GwUserCode::GwUserCode(GwApiInternal *api){
    this->logger=api->getLogger();
    this->api=api;
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
    std::sort(initTasks.begin(),initTasks.end(),[](const GwUserTask &a, const GwUserTask &b){
        return a.order < b.order;    
    });
    LOG_DEBUG(GwLog::DEBUG,"starting %d user init tasks",initTasks.size());
    for (auto it=initTasks.begin();it != initTasks.end();it++){
        LOG_DEBUG(GwLog::LOG,"starting user init task %s with id %d",it->name.c_str(),baseId);
        it->api=new TaskApi(api,baseId,mainLock,it->name,taskData,true);
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
void GwUserCode::handleWebRequest(const String &url,AsyncWebServerRequest *req){
    int sep1=url.indexOf('/');
    String tname;
    if (sep1 > 0){
        tname=url.substring(0,sep1);
        for (auto &&it:userTasks){
            if (it.api && it.name == tname){
                if (it.api->handleWebRequest(url.substring(sep1+1),req)) return;
                break;
            }
        }
    }
    LOG_DEBUG(GwLog::DEBUG,"no task found for web request %s[%s]",url.c_str(),tname.c_str());
    req->send(404, "text/plain", "not found");
}