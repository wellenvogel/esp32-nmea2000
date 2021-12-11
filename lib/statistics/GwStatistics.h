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
    unsigned long *current=NULL;
    unsigned long start=0;
    unsigned long lastLog=0;
    unsigned long lastLogCount=0;
    unsigned long count=0;
    unsigned long max=0;
    size_t len;
    TimeMonitor(size_t len,double factor=0.3){
      this->len=len;
      times=new TimeAverage*[len];
      for (size_t i=0;i<len;i++){
          times[i]=new TimeAverage(factor);
      }
      current=new unsigned long[len];
      reset();
      count=0;
    }
    void reset(){
      start=millis();
      for (size_t i=0;i<len;i++) current[i]=0;
      count++;
      
    }
    String getLog(){
      unsigned long now=millis();
      String log;  
      if (lastLog != 0){
          unsigned long num=count-lastLogCount;
          unsigned long tdif=now-lastLog;
          if (tdif > 0){
            log+=String((double)(num*1000)/(double)tdif);
            log+="/s[";
            log+=String(max);
            log+="ms]#";
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
      unsigned long sv=start;
      for (size_t i=0;i<index;i++){
        if (current[i] != 0) sv=current[i];
      }
      unsigned long now=millis();
      current[index]=now;
      unsigned long currentv=now-sv;
      if ((now-start) > max) max=now-start;
      times[index-1]->add(currentv);
    }
};
