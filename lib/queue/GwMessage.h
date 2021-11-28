#ifndef _GWMESSAGE_H
#define _GWMESSAGE_H
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "GwLog.h"
#include "esp_task_wdt.h"

#ifdef GW_MESSAGE_DEBUG_ENABLED
#define GW_MESSAGE_DEBUG(...) Serial.printf(__VA_ARGS__);
#else
#define GW_MESSAGE_DEBUG(...) 
#endif

/**
 * a message class intended to be send from one task to another
 */
class GwMessage{
  private:
    SemaphoreHandle_t locker;
    SemaphoreHandle_t notifier;
    int refcount=0;
    String name;
  protected:
    virtual void processImpl()=0;
    virtual ~GwMessage();
  public:
    GwMessage(String name=F("unknown"));
    void unref();
    void ref();
    int wait(int maxWaitMillis);
    void process();
    String getName();  
};

/**
 * a class to executa an async web request that returns a string
 */
class GwRequestMessage : public GwMessage{
  protected:
    String result;
    String contentType;
  private:  
    int len=0;
    int consumed=0;
    bool handled=false;
  protected:
    virtual void processRequest()=0;  
    virtual void processImpl();
    virtual ~GwRequestMessage();
  public:
    GwRequestMessage(String contentType,String name=F("web"));
    String getResult(){return result;}
    int getLen(){return len;}
    int consume(uint8_t *destination,int maxLen);
    bool isHandled(){return handled;}
    String getContentType(){
      return contentType;
    }

};

class GwRequestQueue{
  private:
    QueueHandle_t theQueue;
    GwLog *logger;
  public:
    typedef enum{
      MSG_OK,
      MSG_ERR,
      MSG_TIMEOUT
    } MessageSendStatus;
    GwRequestQueue(GwLog *logger,int len);  
    ~GwRequestQueue();
    //both send methods will increment the ref count when enqueing
    MessageSendStatus sendAndForget(GwMessage *msg);
    MessageSendStatus sendAndWait(GwMessage *msg,unsigned long waitMillis);
    GwMessage* fetchMessage(unsigned long waitMillis);
};

#endif