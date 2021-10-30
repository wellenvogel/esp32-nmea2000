#ifndef _GWMESSAGE_H
#define _GWMESSAGE_H
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "esp_task_wdt.h"

#ifdef GW_MESSAGE_DEBUG_ENABLED
#define GW_MESSAGE_DEBUG(...) Serial.printf(__VA_ARGS__);
#else
#define GW_MESSAGE_DEBUG(...) 
#endif

/**
 * a message class intended to be send from one task to another
 */
class Message{
  private:
    SemaphoreHandle_t locker;
    SemaphoreHandle_t notifier;
    int refcount=0;
  protected:
    virtual void processImpl()=0;
  public:
    Message(){
      locker=xSemaphoreCreateMutex();
      notifier=xSemaphoreCreateCounting(1,0);
      refcount=1;
      GW_MESSAGE_DEBUG("Message: %p\n",this);
    }
    virtual ~Message(){
      GW_MESSAGE_DEBUG("~Message %p\n",this);
      vSemaphoreDelete(locker);
      vSemaphoreDelete(notifier);
    }
    void unref(){
      GW_MESSAGE_DEBUG("Message::unref %p\n",this);
      bool mustDelete=false;
      xSemaphoreTake(locker,portMAX_DELAY);
      if (refcount > 0){
        refcount--;
        if (refcount == 0) mustDelete=true;
      }
      xSemaphoreGive(locker);
      if (mustDelete){
        delete this;
      }
    }
    void ref(){
      xSemaphoreTake(locker,portMAX_DELAY);
      refcount++;
      xSemaphoreGive(locker);
    }
    int wait(int maxWaitMillis){
      static const int maxWait=1000;
      int maxRetries=maxWaitMillis/maxWait;
      int lastWait=maxWaitMillis - maxWait*maxRetries;
      for (int retries=maxRetries;retries>0;retries--){
        if (xSemaphoreTake(notifier,pdMS_TO_TICKS(maxWait))) return true;
        esp_task_wdt_reset();
      }
      if (lastWait){
        return xSemaphoreTake(notifier,pdMS_TO_TICKS(maxWait));
      }
      return false;
    }
    void process(){
      GW_MESSAGE_DEBUG("Message::process %p\n",this);
      processImpl();
      xSemaphoreGive(notifier);
    }  
};

/**
 * a class to executa an async web request that returns a string
 */
class RequestMessage : public Message{
  protected:
    String result;
  private:  
    int len=0;
    int consumed=0;
    bool handled=false;
  protected:
    virtual void processRequest()=0;  
    virtual void processImpl(){
      GW_MESSAGE_DEBUG("RequestMessage processImpl(1) %p\n",this);
      processRequest();
      GW_MESSAGE_DEBUG("RequestMessage processImpl(2) %p\n",this);
      len=strlen(result.c_str());
      consumed=0;
      handled=true;
    }
  public:
    RequestMessage():Message(){
    }
    virtual ~RequestMessage(){
      GW_MESSAGE_DEBUG("~RequestMessage %p\n",this)
    }
    String getResult(){return result;}
    int getLen(){return len;}
    int consume(uint8_t *destination,int maxLen){
      if (!handled) return RESPONSE_TRY_AGAIN;
      if (consumed >= len) return 0;
      int cplen=maxLen;
      if (cplen > (len-consumed)) cplen=len-consumed;
      memcpy(destination,result.c_str()+consumed,cplen);
      consumed+=cplen;
      return cplen; 
    }
    bool isHandled(){return handled;}

};

#endif