#pragma once
#include <Arduino.h>
class TimeAverage{
  double factor=0.3;
  double current=0;
  unsigned long count=0;
  unsigned long max=0;
  bool init=false;
  public:
    TimeAverage(double factor){
      this->factor=factor;
    }
    void add(unsigned long diff){
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
    unsigned long getMax(){
        return max;
    }

};
class TimeMonitor{
  public:
    TimeAverage **times=NULL;
    unsigned long start=0;
    unsigned long lastLog=0;
    unsigned long lastLogCount=0;
    unsigned long count=0;
    size_t len;
    TimeMonitor(size_t len,double factor=0.3){
      this->len=len;
      times=new TimeAverage*[len];
      for (size_t i=0;i<len;i++){
          times[i]=new TimeAverage(factor);
      }
      reset();
      count=0;
    }
    void reset(){
      start=millis();
      count++;
      
    }
    double getMax(){
        double rt=0;
        for (size_t i=0;i<len;i++){
            if (times[i]->getCurrent() == 0) continue;
            if (times[i]->getCurrent() > rt) rt=times[i]->getCurrent();
        }
        return rt;
    }
    String getLog(){
      unsigned long now=millis();
      String log;  
      if (lastLog != 0){
          unsigned long num=count-lastLogCount;
          unsigned long tdif=now-lastLog;
          if (tdif > 0){
            log+=String((double)(num*1000)/(double)tdif);
            log+="/s,";
            log+=String(getMax());
            log+="ms#";
          }
      }
      lastLog=now;
      lastLogCount=count; 
      for (size_t i=1;i<len;i++){
        if (times[i]->getCount()){
          log+=String(i);
          log+=":";
          log+=String(times[i]->getCurrent());
          log+="[";
          log+=String(times[i]->getMax());
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
      unsigned long current=millis()-start;
      times[index-1]->add(current);
    }
};
