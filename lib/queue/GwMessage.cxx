#include "GwMessage.h"
GwMessage::GwMessage(String name)
{
    this->name = name;
    locker = xSemaphoreCreateMutex();
    notifier = xSemaphoreCreateCounting(1, 0);
    refcount = 1;
    GW_MESSAGE_DEBUG("Message: %p\n", this);
}
GwMessage::~GwMessage()
{
    GW_MESSAGE_DEBUG("~Message %p\n", this);
    vSemaphoreDelete(locker);
    vSemaphoreDelete(notifier);
}
void GwMessage::unref()
{
    GW_MESSAGE_DEBUG("Message::unref %p\n", this);
    bool mustDelete = false;
    xSemaphoreTake(locker, portMAX_DELAY);
    if (refcount > 0)
    {
        refcount--;
        if (refcount == 0)
            mustDelete = true;
    }
    xSemaphoreGive(locker);
    if (mustDelete)
    {
        delete this;
    }
}
void GwMessage::ref()
{
    xSemaphoreTake(locker, portMAX_DELAY);
    refcount++;
    xSemaphoreGive(locker);
}
int GwMessage::wait(int maxWaitMillis)
{
    static const int maxWait = 1000;
    int maxRetries = maxWaitMillis / maxWait;
    int lastWait = maxWaitMillis - maxWait * maxRetries;
    for (int retries = maxRetries; retries > 0; retries--)
    {
        if (xSemaphoreTake(notifier, pdMS_TO_TICKS(maxWait)))
            return true;
        esp_task_wdt_reset();
    }
    if (lastWait)
    {
        return xSemaphoreTake(notifier, pdMS_TO_TICKS(maxWait));
    }
    return false;
}
void GwMessage::process()
{
    GW_MESSAGE_DEBUG("Message::process %p\n", this);
    processImpl();
    xSemaphoreGive(notifier);
}
String GwMessage::getName() { return name; }

void GwRequestMessage::processImpl(){
      GW_MESSAGE_DEBUG("RequestMessage processImpl(1) %p\n",this);
      processRequest();
      GW_MESSAGE_DEBUG("RequestMessage processImpl(2) %p\n",this);
      len=strlen(result.c_str());
      consumed=0;
      handled=true;
    }
GwRequestMessage::GwRequestMessage(String contentType, String name):GwMessage(name){
      this->contentType=contentType;
    }
GwRequestMessage::~GwRequestMessage(){
      GW_MESSAGE_DEBUG("~RequestMessage %p\n",this)
    }
int GwRequestMessage::consume(uint8_t *destination,int maxLen){
      if (!handled) return RESPONSE_TRY_AGAIN;
      if (consumed >= len) return 0;
      int cplen=maxLen;
      if (cplen > (len-consumed)) cplen=len-consumed;
      memcpy(destination,result.c_str()+consumed,cplen);
      consumed+=cplen;
      return cplen; 
    }
GwRequestQueue::GwRequestQueue(GwLog *logger,int len){
    theQueue=xQueueCreate(len,sizeof(GwMessage*));
    this->logger=logger;
}
GwRequestQueue::~GwRequestQueue(){
    vQueueDelete(theQueue);
}

GwRequestQueue::MessageSendStatus GwRequestQueue::sendAndForget(GwMessage *msg){
    msg->ref(); //for the queue
    if (!xQueueSend(theQueue, &msg, 0))
    {
        LOG_DEBUG(GwLog::LOG,"unable to enqueue %s",msg->getName().c_str());
        msg->unref();
        return MSG_ERR;
    }
    return MSG_OK;
}
GwRequestQueue::MessageSendStatus GwRequestQueue::sendAndWait(GwMessage *msg,unsigned long waitMillis){
    msg->ref(); //for the queue
    msg->ref(); //for us waiting
    if (!xQueueSend(theQueue, &msg, 0))
    {
        LOG_DEBUG(GwLog::LOG,"unable to enqueue %s",msg->getName().c_str());
        msg->unref();
        msg->unref();
        return MSG_ERR;
    }
    LOG_DEBUG(GwLog::DEBUG + 1, "wait queue for %s",msg->getName().c_str());
    if (msg->wait(waitMillis)){
       LOG_DEBUG(GwLog::DEBUG + 1, "request ok for %s",msg->getName().c_str()); 
       msg->unref();
       return MSG_OK;
    }
    LOG_DEBUG(GwLog::DEBUG, "request timeout for %s",msg->getName().c_str()); 
    msg->unref();
    return MSG_TIMEOUT;
}
GwMessage* GwRequestQueue::fetchMessage(unsigned long waitMillis){
    GwMessage *msg=NULL;
    if (xQueueReceive(theQueue,&msg,waitMillis)){
        return msg;
    }
    return NULL;
}