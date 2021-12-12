#pragma once
#include <Arduino.h>
class TimeAverage{
  double factor=0.3;
  double current=0;
  unsigned long count=0;
  int64_t max=0;
  bool init=false;
  public:
    TimeAverage(double factor){
      this->factor=factor;
    }
    void add(int64_t diff){
      if (diff > max) max=diff;  
      count++;
      if (! init){
          current=(double)diff;
          init=true;
      }
      else{
          double add=((double)diff - current)*factor;
          current+=add;
      }
    }
    double getCurrent(){
      return current;
    }

    unsigned long getCount(){
      return count;
    }
    void resetMax(){
        max=0;
    }
    int64_t getMax(){
        return max;
    }

};
class TimeMonitor{
  public:
    TimeAverage **times=NULL;
    TimeAverage *loop=NULL;
    int64_t *current=NULL;
    int64_t start=0;
    int64_t last=0;
    unsigned long lastLog=0;
    unsigned long lastLogCount=0;
    unsigned long count=0;
    int64_t max=0;
    size_t len;
    ~TimeMonitor(){
        for (size_t i=0;i<len;i++){
          delete times[i];
        }
        delete times;
        delete current;
        delete loop;
    }
    TimeMonitor(size_t len,double factor=0.3){
      this->len=len;
      loop=new TimeAverage(factor);
      times=new TimeAverage*[len];
      for (size_t i=0;i<len;i++){
          times[i]=new TimeAverage(factor);
      }
      current=new int64_t[len];
      reset();
      count=0;
    }
    void reset(){
      if (last != 0 && start != 0) loop->add(last-start);
      start=esp_timer_get_time();
      for (size_t i=0;i<len;i++) current[i]=0;
      count++;
      
    }
    String getLog(){
      int64_t now=millis();
      String log;  
      if (lastLog != 0){
          unsigned long num=count-lastLogCount;
          unsigned long tdif=now-lastLog;
          if (tdif > 0){
            log+=String((double)(num*1000)/(double)tdif);
            log+="/s";
            log+=String(loop->getCurrent());
            log+="[";
            log+=String((unsigned long)max);
            log+="us]#";
          }
      }
      lastLog=now;
      lastLogCount=count;
      max=0; 
      for (size_t i=1;i<len;i++){
        if (times[i]->getCount()){
          log+=String(i);
          log+=":";
          log+=String(times[i]->getCurrent());
          log+="[";
          log+=String((unsigned long)(times[i]->getMax()));
          log+="],";
        }
      }
      for (size_t i=0;i<len;i++){
          times[i]->resetMax();
      }
      return log;
    }
    void setTime(size_t index){
      if (index < 1 || index >= len) return;
      int64_t sv=start;
      for (size_t i=0;i<index;i++){
        if (current[i] != 0) sv=current[i];
      }
      int64_t now=esp_timer_get_time();
      last=now;
      current[index]=now;
      int64_t currentv=now-sv;
      if ((now-start) > max) max=now-start;
      times[index]->add(currentv);
    }
};
