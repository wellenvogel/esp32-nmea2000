#ifndef _GWCOUNTER_H
#define _GWCOUNTER_H
#include <map>
#include "ArduinoJson.h"
template<class T> class GwCounter{
    private:
        typedef std::map<T,unsigned long> CounterMap;
        CounterMap okCounter;
        CounterMap failCounter;
        unsigned long globalOk=0;
        unsigned long globalFail=0;
        String name;
    public:
        GwCounter(String name){
            this->name=name;
        };
        void reset(){
            okCounter.clear();
            failCounter.clear();
            globalFail=0;
            globalOk=0;
        }
        void add(T key){
            globalOk++;
            auto it=okCounter.find(key);
            if (it == okCounter.end()){
                okCounter[key]=1;
            }
            else{
                it->second++;
            }
        }
        void addFail(T key){
            globalFail++;
            auto it=failCounter.find(key);
            if (it == failCounter.end()){
                failCounter[key]=1;
            }
            else{
                it->second++;
            }
        }
        int getJsonSize(){
            return JSON_OBJECT_SIZE(4)+JSON_OBJECT_SIZE(okCounter.size()+1)+
                JSON_OBJECT_SIZE(failCounter.size()+1)+
                okCounter.size()*5+failCounter.size()*5;
        }
        void toJson(JsonDocument &json){
            JsonObject jo=json.createNestedObject(name);
            jo[F("sumOk")]=globalOk;
            jo[F("sumFail")]=globalFail;
            JsonObject jok=jo.createNestedObject(F("ok"));
            for (auto it=okCounter.begin();it!=okCounter.end();it++){
                jok[String(it->first)]=it->second;
            }
            JsonObject jfail=jo.createNestedObject(F("fail"));
            for (auto it=failCounter.begin();it!=failCounter.end();it++){
                jfail[String(it->first)]=it->second;
            }
        }
};
#endif