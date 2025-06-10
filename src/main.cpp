/*
  (C) Andreas Vogel andreas@wellenvogel.de
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "GwAppInfo.h"
// #define GW_MESSAGE_DEBUG_ENABLED
//#define FALLBACK_SERIAL
#define OWN_LOOP
const unsigned long HEAP_REPORT_TIME=2000; //set to 0 to disable heap reporting
#include <Arduino.h>
#include "Preferences.h"
#include "GwApi.h"
#define GW_PINDEFS
#include "GwHardware.h"

#ifndef N2K_LOAD_LEVEL
#define N2K_LOAD_LEVEL 0
#endif

#ifndef N2K_CERTIFICATION_LEVEL
#define N2K_CERTIFICATION_LEVEL 0xff
#endif

#include <ActisenseReader.h>
#include <Seasmart.h>
#include <N2kMessages.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <memory>
#include <map>
#include <vector>
#include "esp_heap_caps.h"
#include <MD5Builder.h>
#include "GwJsonDocument.h"
#include "N2kDataToNMEA0183.h"
#include <esp_image_format.h>


#include "GwLog.h"
#include "GWConfig.h"
#include "GWWifi.h"
#include "GwSocketServer.h"
#include "GwBoatData.h"
#include "GwMessage.h"
#include "GwSerial.h"
#include "GwWebServer.h"
#include "NMEA0183DataToN2K.h"
#include "GwCounter.h"
#include "GwXDRMappings.h"
#include "GwSynchronized.h"
#include "GwUserCode.h"
#include "GwStatistics.h"
#include "GwUpdate.h"
#include "GwTcpClient.h"
#include "GwChannel.h"
#include "GwChannelList.h"
#include "GwTimer.h"


#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500
#define MAX_NMEA0183_MESSAGE_SIZE MAX_NMEA2000_MESSAGE_SEASMART_SIZE
//assert length of firmware name and version
CASSERT(strlen(FIRMWARE_TYPE) <= 32, "environment name (FIRMWARE_TYPE) must not exceed 32 chars");
CASSERT(strlen(VERSION) <= 32, "VERSION must not exceed 32 chars");
CASSERT(strlen(IDF_VERSION) <= 32,"IDF_VERSION must not exceed 32 chars");
//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/app_image_format.html
//and removed the bugs in the doc...
__attribute__((section(".rodata_custom_desc"))) esp_app_desc_t custom_app_desc = { 
  ESP_APP_DESC_MAGIC_WORD,
  1,
  {0,0},
  VERSION,
  FIRMWARE_TYPE,
  "00:00:00",
  "2021/12/13",
  IDF_VERSION,
  {},
  {}
};


String firmwareType(FIRMWARE_TYPE);

typedef std::map<String,String> StringMap;


GwLog logger(LOGLEVEL,NULL);
GwConfigHandler config(&logger);

#include "Nmea2kTwai.h"
static const unsigned long CAN_RECOVERY_PERIOD=3000; //ms
static const unsigned long NMEA2000_HEARTBEAT_INTERVAL=5000;
class Nmea2kTwaiLog : public Nmea2kTwai{
  private:
    GwLog* logger;
  public:
    Nmea2kTwaiLog(gpio_num_t _TxPin,  gpio_num_t _RxPin, unsigned long recoveryPeriod,GwLog *l):
      Nmea2kTwai(_TxPin,_RxPin,recoveryPeriod,recoveryPeriod),logger(l){}
    virtual void logDebug(int level, const char *fmt,...){
      va_list args;
      va_start(args,fmt);
      if (level > 2) level++; //error+info+debug are similar, map msg to 4
      logger->logDebug(level,fmt,args);
    }
};

#ifndef ESP32_CAN_TX_PIN
 #pragma message "WARNING: ESP32_CAN_TX_PIN not defined"
 #define ESP32_CAN_TX_PIN GPIO_NUM_NC
#endif
#ifndef ESP32_CAN_RX_PIN
 #pragma message "WARNING: ESP32_CAN_RX_PIN not defined"
 #define ESP32_CAN_RX_PIN GPIO_NUM_NC
#endif

Nmea2kTwai &NMEA2000=*(new Nmea2kTwaiLog((gpio_num_t)ESP32_CAN_TX_PIN,(gpio_num_t)ESP32_CAN_RX_PIN,CAN_RECOVERY_PERIOD,&logger));

#ifdef GWBUTTON_PIN
bool fixedApPass=false;
#else
#ifdef FORCE_AP_PWCHANGE
bool fixedApPass=false;
#else
bool fixedApPass=true;
#endif
#endif
GwWifi gwWifi(&config,&logger,fixedApPass);
GwChannelList channels(&logger,&config);
GwBoatData boatData(&logger,&config);
GwXDRMappings xdrMappings(&logger,&config);
bool sendOutN2k=true;


int NodeAddress;  // To store last Node Address

Preferences preferences;             // Nonvolatile storage on ESP32 - To store LastDeviceAddress
N2kDataToNMEA0183 *nmea0183Converter=NULL;
NMEA0183DataToN2K *toN2KConverter=NULL;
SemaphoreHandle_t mainLock;


GwRequestQueue mainQueue(&logger,20);
GwWebServer webserver(&logger,&mainQueue,80);

GwCounter<unsigned long> countNMEA2KIn("countNMEA2000in");
GwCounter<unsigned long> countNMEA2KOut("countNMEA2000out");
GwIntervalRunner timers;

bool checkPass(String hash){
  return config.checkPass(hash);
}


GwUpdate updater(&logger,&webserver,&checkPass);
GwConfigInterface *systemName=config.getConfigItem(config.systemName,true);


void handleN2kMessage(const tN2kMsg &n2kMsg,int sourceId, bool isConverted=false)
{
  logger.logDebug(GwLog::DEBUG + 1, "N2K: pgn %d, dir %d", 
    n2kMsg.PGN,sourceId);
  if (sourceId == N2K_CHANNEL_ID){
    countNMEA2KIn.add(n2kMsg.PGN);
  }
  char *buf=new char[MAX_NMEA2000_MESSAGE_SEASMART_SIZE+3];
  std::unique_ptr<char> bufDel(buf);
  bool messageCreated=false;
  channels.allChannels([&](GwChannel *c){
    if (c->sendSeaSmart()){
      if (! messageCreated){
        size_t len;
        if ((len=N2kToSeasmart(n2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE)) != 0) {
          buf[len]=0x0d;
          len++;
          buf[len]=0x0a;
          len++;
          buf[len]=0;
          messageCreated=true;
        }
      }
      if (messageCreated){
        c->sendToClients(buf,sourceId,true);
      }
    }
  });
  
  channels.allChannels([&](GwChannel *c){
    c->sendActisense(n2kMsg,sourceId);
  });
  if (! isConverted){
    nmea0183Converter->HandleMsg(n2kMsg,sourceId);
  }
  if (sourceId != N2K_CHANNEL_ID && sendOutN2k){
    if (NMEA2000.SendMsg(n2kMsg)){
      countNMEA2KOut.add(n2kMsg.PGN);
    }
    else{
      countNMEA2KOut.addFail(n2kMsg.PGN);
    }
  }
};


//*****************************************************************************
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg, int sourceId,bool convert=false) {
  logger.logDebug(GwLog::DEBUG+2,"SendNMEA0183(1)");
  char *buf=new char[MAX_NMEA0183_MESSAGE_SIZE+3];
  std::unique_ptr<char> bufDel(buf);
  if ( !NMEA0183Msg.GetMessage(buf, MAX_NMEA0183_MESSAGE_SIZE) ) return;
  logger.logDebug(GwLog::DEBUG+2,"SendNMEA0183: %s",buf);
  if (convert){
    toN2KConverter->parseAndSend(buf,sourceId);
  }
  size_t len=strlen(buf);
  buf[len]=0x0d;
  buf[len+1]=0x0a;
  buf[len+2]=0;
  channels.allChannels([&](GwChannel *c){
    c->sendToClients(buf,sourceId,false);
  });
}

class CalibrationValues {
  using Map=std::map<String,double>;
  Map values;
  SemaphoreHandle_t lock=nullptr;
  public:
    CalibrationValues(){
      lock=xSemaphoreCreateMutex();
    }
    void set(const String &name,double value){
      GWSYNCHRONIZED(lock);
      values[name]=value;
    }
    bool get(const String &name, double &value){
      GWSYNCHRONIZED(lock);
      auto it=values.find(name);
      if (it==values.end()) return false;
      value=it->second;
      return true;
    }
};

class ApiImpl : public GwApiInternal
{
private:
  int sourceId = -1;
  std::unique_ptr<CalibrationValues> calibrations;

public:
  ApiImpl(int sourceId)
  {
    this->sourceId = sourceId;
    calibrations.reset(new CalibrationValues());
  }
  virtual GwRequestQueue *getQueue()
  {
    return &mainQueue;
  }
  virtual void sendN2kMessage(const tN2kMsg &msg,bool convert)
  {
    handleN2kMessage(msg,sourceId,!convert);
    
  }
  virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, int sourceId,bool convert)
  {
      SendNMEA0183Message(msg, sourceId,convert);
  }
  virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, bool convert)
  {
      SendNMEA0183Message(msg, sourceId,convert);
  }
  virtual int getSourceId()
  {
    return sourceId;
  };
  virtual GwConfigHandler *getConfig()
  {
    return &config;
  }
  virtual GwLog* getLogger(){
    return &logger;
  }
  virtual void getBoatDataValues(int numValues,BoatValue **list){
    for (int i=0;i<numValues;i++){
      GwBoatItemBase *item=boatData.getBase(list[i]->getName());
      list[i]->changed=false;
      if (item){
        bool newValid=item->isValid();
        if (newValid != list[i]->valid) list[i]->changed=true;
        list[i]->valid=newValid;
        if (newValid){
          double newValue=item->getDoubleValue();
          if (newValue != list[i]->value) list[i]->changed=true;
          list[i]->value=newValue;
          int newSource=item->getLastSource();
          if (newSource != list[i]->source){
            list[i]->source=newSource;
            list[i]->changed=true;
          }
        }
        list[i]->setFormat(item->getFormat());
      }
      else{
        if (list[i]->valid) list[i]->changed=true;
        list[i]->valid=false;
      }
    }
  }
  virtual void getStatus(Status &status){
    status.empty();
    status.wifiApOn=gwWifi.isApActive();
    status.wifiClientOn=gwWifi.isClientActive();
    status.wifiClientConnected=gwWifi.clientConnected();
    status.wifiApIp=gwWifi.apIP();
    status.systemName=systemName->asString();
    status.wifiApPass=config.getString(config.apPassword);
    status.wifiClientIp=WiFi.localIP().toString();
    status.wifiClientSSID=config.getString(config.wifiSSID);
    status.n2kRx=countNMEA2KIn.getGlobal();
    status.n2kTx=countNMEA2KOut.getGlobal();
    channels.fillStatus(status);
  }
  virtual GwBoatData *getBoatData(){
    return &boatData;
  }
  virtual const char* getTalkerId(){
    return config.getCString(config.talkerId,"GP");
  }
  virtual ~ApiImpl(){}
  virtual TaskInterfaces *taskInterfaces(){ return nullptr;}
  virtual bool addXdrMapping(const GwXDRMappingDef &mapping){
    if (! config.userChangesAllowed()){
      logger.logDebug(GwLog::ERROR,"trying to add an XDR mapping %s after the init phase",mapping.toString().c_str());
      return false;
    }
    return xdrMappings.addFixedMapping(mapping);
  }
  virtual void registerRequestHandler(const String &url,HandlerFunction handler){
  }
  virtual void addCapability(const String &name, const String &value){}
  virtual bool addUserTask(GwUserTaskFunction task,const String Name, int stackSize=2000){
    return false;
  }
  virtual void setCalibrationValue(const String &name, double value){
    calibrations->set(name,value);
  }

  bool getCalibrationValue(const String &name,double &value){
    return calibrations->get(name,value);
  }
};

bool delayedRestart(){
  return xTaskCreate([](void *p){
    GwLog *logRef=(GwLog *)p;
    logRef->logDebug(GwLog::LOG,"delayed reset started");
    delay(800);
    ESP.restart();
    vTaskDelete(NULL);
  },"reset",2000,&logger,0,NULL) == pdPASS;
}
ApiImpl *apiImpl=new ApiImpl(MIN_USER_TASK);
GwUserCode userCodeHandler(apiImpl);

#define JSON_OK "{\"status\":\"OK\"}"
#define JSON_INVALID_PASS F("{\"status\":\"invalid password\"}")

//WebServer requests that should
//be processed inside the main loop
//this prevents us from the need to sync all the accesses
class ResetRequest : public GwRequestMessage
{
  String hash;
public:
  ResetRequest(String hash) : GwRequestMessage(F("application/json"),F("reset")){
    this->hash=hash;
  };

protected:
  virtual void processRequest()
  {
    logger.logDebug(GwLog::LOG, "Reset Button");
    if (! checkPass(hash)){
      result=JSON_INVALID_PASS;
      return;
    }
    result = JSON_OK;
    if (!delayedRestart()){
      logger.logDebug(GwLog::ERROR,"cannot initiate restart");
    }
  }
};

class StatusRequest : public GwRequestMessage
{
public:
  StatusRequest() : GwRequestMessage(F("application/json"),F("status")){};

protected:
  virtual void processRequest()
  {
    GwJsonDocument status(305 + 
      countNMEA2KIn.getJsonSize()+
      countNMEA2KOut.getJsonSize() +
      channels.getJsonSize()+
      userCodeHandler.getJsonSize()
      );
    status["version"] = VERSION;
    status["wifiConnected"] = gwWifi.clientConnected();
    status["wifiSSID"] = config.getString(GwConfigDefinitions::wifiSSID);
    status["clientIP"] = WiFi.localIP().toString();
    status["apIp"] = gwWifi.apIP();
    size_t bsize=2*sizeof(unsigned long)+1;
    unsigned long base=config.getSaltBase() + ( millis()/1000UL & ~0x7UL);
    char buffer[bsize];
    GwConfigHandler::toHex(base,buffer,bsize);
    status["salt"] = buffer;
    status["fwtype"]= firmwareType;
    status["chipid"]=CONFIG_IDF_FIRMWARE_CHIP_ID;
    status["heap"]=(long)xPortGetFreeHeapSize();
    Nmea2kTwai::Status n2kState=NMEA2000.getStatus();
    Nmea2kTwai::STATE driverState=n2kState.state;
    if (driverState == Nmea2kTwai::ST_RUNNING){
      unsigned long lastRec=NMEA2000.getLastRecoveryStart();
      if (lastRec > 0 && (lastRec+NMEA2000_HEARTBEAT_INTERVAL*2) > millis()){
        //we still report bus off at least for 2 heartbeat intervals
        //this avoids always reporting BUS_OFF-RUNNING-BUS_OFF if the bus off condition
        //remains
        driverState=Nmea2kTwai::ST_BUS_OFF;
      }
    }
    status["n2kstate"]=NMEA2000.stateStr(driverState);
    status["n2knode"]=NodeAddress;
    status["minUser"]=MIN_USER_TASK;
    //nmea0183Converter->toJson(status);
    countNMEA2KIn.toJson(status);
    countNMEA2KOut.toJson(status);
    channels.toJson(status);
    userCodeHandler.fillStatus(status);
    serializeJson(status, result);
  }
};

class CheckPassRequest : public GwRequestMessage{
  String hash;
  public:
    CheckPassRequest(String h): GwRequestMessage(F("application/json"),F("checkPass")){
      this->hash=h;
    };
  protected:
    virtual void processRequest(){
      bool res=checkPass(hash);
      if (res) result=JSON_OK;
      else result=F("{\"status\":\"ERROR\"}");
    }

};
class CapabilitiesRequest : public GwRequestMessage{
  public:
    CapabilitiesRequest() : GwRequestMessage(F("application/json"),F("capabilities")){};
  protected:
    virtual void processRequest(){
      int numCapabilities=userCodeHandler.getCapabilities()->size();
      int numSpecial=config.numSpecial();
      logger.logDebug(GwLog::LOG,"capabilities user=%d, config=%d",numCapabilities,numSpecial);
      GwJsonDocument json(JSON_OBJECT_SIZE(numCapabilities*3+numSpecial*2+8));
      for (auto it=userCodeHandler.getCapabilities()->begin();
        it != userCodeHandler.getCapabilities()->end();it++){
          json[it->first]=it->second;
        }
      std::vector<String> specialCfg=config.getSpecial();
      for (auto it=specialCfg.begin();it != specialCfg.end();it++){
        GwConfigInterface *cfg=config.getConfigItem(*it);
        if (cfg){
          logger.logDebug(GwLog::LOG,"config mode %s=%d",it->c_str(),(int)(cfg->getType()));
          json["CFGMODE"+*it]=(int)cfg->getType();
        }
      }
      json["serialmode"]=channels.getMode(SERIAL1_CHANNEL_ID);
      json["serial2mode"]=channels.getMode(SERIAL2_CHANNEL_ID);
      #ifdef GWBUTTON_PIN
      json["hardwareReset"]="true";
      #endif
      json["apPwChange"] = fixedApPass?"false":"true";
      serializeJson(json,result);
    }  
};
class ConverterInfoRequest : public GwRequestMessage{
  public:
    ConverterInfoRequest() : GwRequestMessage(F("application/json"),F("converterInfo")){};
  protected:
    virtual void processRequest(){
      GwJsonDocument json(512);
      String keys=toN2KConverter->handledKeys();
      logger.logDebug(GwLog::DEBUG,"handled nmea0183: %s",keys.c_str());
      json["nmea0183"]=keys;
      keys=nmea0183Converter->handledKeys();
      logger.logDebug(GwLog::DEBUG,"handled nmea2000: %s",keys.c_str());
      json["nmea2000"]=keys;
      serializeJson(json,result);
    }  
};
class ConfigRequest : public GwRequestMessage
{
public:
  ConfigRequest() : GwRequestMessage(F("application/json"),F("config")){};

protected:
  virtual void processRequest()
  {
    result = config.toJson();
  }
};


class ResetConfigRequest : public GwRequestMessage
{
  String hash;
public:
  ResetConfigRequest(String hash) : GwRequestMessage(F("application/json"),F("resetConfig")){
    this->hash=hash;
  };
  virtual int getTimeout(){return 4000;}

protected:
  virtual void processRequest()
  {
    if (! checkPass(hash)){
      result=JSON_INVALID_PASS;
      return;
    }
    config.reset();
    logger.logDebug(GwLog::ERROR,"reset config, restart");
    result = JSON_OK;
    delayedRestart();
  }
};
class BoatDataRequest : public GwRequestMessage
{
public:
  BoatDataRequest() : GwRequestMessage(F("application/json"),F("boatData")){};

protected:
  virtual void processRequest()
  {
    result = boatData.toJson();
  }
};
class BoatDataStringRequest : public GwRequestMessage
{
public:
  BoatDataStringRequest() : GwRequestMessage(F("text/plain"),F("boatDataString")){};

protected:
  virtual void processRequest()
  {
    result = boatData.toString();
  }
};

class XdrExampleRequest : public GwRequestMessage
{
public:
  String mapping;
  double value;
  XdrExampleRequest(String mapping, double value) : GwRequestMessage(F("text/plain"),F("xdrExample")){
    this->mapping=mapping;
    this->value=value;
  };

protected:
  virtual void processRequest()
  {
    String val=xdrMappings.getXdrEntry(mapping,value);
    if (val == "") {
      result=val;
      return;
    }
    tNMEA0183Msg msg;
    msg.Init("XDR",config.getString(config.talkerId,String("GP")).c_str());
    msg.AddStrField(val.c_str());
    char *buf=new char[MAX_NMEA0183_MSG_BUF_LEN+2];
    std::unique_ptr<char> bufDel(buf);
    msg.GetMessage(buf,MAX_NMEA0183_MSG_BUF_LEN);
    result=buf;
  }
};
class XdrUnMappedRequest : public GwRequestMessage
{
public:
  XdrUnMappedRequest() : GwRequestMessage(F("text/plain"),F("boatData")){};

protected:
  virtual void processRequest()
  {
    result = xdrMappings.getUnMapped();
  }
};


void handleConfigRequestData(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  typedef struct{
    char notFirst;
    char hashChecked;
    char parsingValue;
    int bName;
    char name[33];
    int bValue;
    char value[512];
  }RequestNV;
  long long lastRecords=logger.getRecordCounter();
  logger.logDebug(GwLog::DEBUG,"handleConfigRequestData len=%d,idx=%d,total=%d",(int)len,(int)index,(int)total);
  if (request->_tempObject == NULL){
    logger.logDebug(GwLog::DEBUG,"handleConfigRequestData create receive struct");
    //we cannot use new here as it will be deleted with free
    request->_tempObject=malloc(sizeof(RequestNV));
    memset(request->_tempObject,0,sizeof(RequestNV));
  }
  RequestNV *nv=(RequestNV*)(request->_tempObject);
  if (nv->notFirst && ! nv->hashChecked){
    return; //ignore data
  }
  int parsed=0;
  while (parsed < len)
  {
    if (!nv->parsingValue)
    {
      int maxSize = sizeof(RequestNV::name) - 1;
      if (nv->bName >= maxSize)
      {
        nv->name[maxSize] = 0;
        logger.logDebug(GwLog::DEBUG, "parse error name too long %s", nv->name);
        nv->bName = 0;
      }
      while (nv->bName < maxSize && parsed < len)
      {
        bool endName = *data == '=';
        nv->name[nv->bName] = endName ? 0 : *data;
        nv->bName++;
        parsed++;
        data++;
        if (endName)
        {
          nv->parsingValue = 1;
          break;
        }
      }
    }
    bool valueDone = false;
    if (nv->parsingValue)
    {
      int maxSize = sizeof(RequestNV::value) - 1;
      if (nv->bValue >= maxSize)
      {
        nv->value[maxSize] = 0;
        logger.logDebug(GwLog::DEBUG, "parse error value too long %s:%s", nv->name, nv->value);
        nv->bValue = 0;
      }
      while (nv->bValue < maxSize && parsed < len)
      {
        valueDone = *data == '&';
        nv->value[nv->bValue] = valueDone ? 0 : *data;
        nv->bValue++;
        parsed++;
        data++;
        if (valueDone) break;
      }
      if (! valueDone){
        if (parsed >= len && (len+index) >= total){
          //request ends here
          nv->value[nv->bValue]=0;
          valueDone=true;
        } 
      }
      if (valueDone){
        String name(nv->name);
        String value(nv->value);
        if (! nv->notFirst){
          nv->notFirst=1;
          //we expect the _hash as first parameter
          if (name != String("_hash")){
            logger.logDebug(GwLog::ERROR,"missing first parameter _hash in setConfig");
            request->send(200,"application/json","{\"status\":\"missing _hash\"}");
            return;
          }
          if (! config.checkPass(request->urlDecode(value))){
            request->send(200,"application/json",JSON_INVALID_PASS);
            return;
          }
          else{
            nv->hashChecked=1;
          }
        }
        else{
          if (nv->hashChecked){
            logger.logDebug(GwLog::DEBUG,"value ns=%d,n=%s,vs=%d,v=%s",nv->bName,nv->name,nv->bValue,nv->value);
            if ((logger.getRecordCounter() - lastRecords) > 20){
              logger.flush();
              lastRecords=logger.getRecordCounter();
            }
            config.updateValue(request->urlDecode(name),request->urlDecode(value));
          }
        }
        nv->parsingValue=0;
        nv->bName=0;
        nv->bValue=0;
      }
    }
  }
  if (parsed >= len && (len+index)>= total){
    if (nv->notFirst){
      if (nv->hashChecked){
        request->send(200,"application/json",JSON_OK);
        logger.flush();
        logger.logDebug(GwLog::DEBUG,"Heap free=%ld, minFree=%ld",
          (long)xPortGetFreeHeapSize(),
          (long)xPortGetMinimumEverFreeHeapSize()
        );
        logger.flush();
        delayedRestart();
      }
    }
    else{
      request->send(200,"application/json","{\"status\":\"missing _hash\"}");
    }
  }
}

TimeMonitor monitor(20,0.2);
class DefaultLogWriter: public GwLogWriter{
    public:
        virtual ~DefaultLogWriter(){};
        virtual void write(const char *data){
            USBSerial.print(data);
        }
};
void loopRun();
void loop(){
  #ifdef OWN_LOOP
  vTaskDelete(NULL);
  return;
  #else
  loopRun();
  #endif
}
void loopFunction(void *){
  while (true){
    loopRun();
    //we don not call the serialEvent stuff as in the original
    //main loop as this could cause some sort of a deadlock
    //if serial writing or reading is done in a different thread
    //and it remains inside some read/write routine with the uart being
    //locked
    //if(Serial1.available()) {}
    //if(Serial.available()) {}
    //if(Serial2.available()) {}
    //delay(1);
  }
}
const String USERPREFIX="/api/user/";
void setup() {
  mainLock=xSemaphoreCreateMutex();
  uint8_t chipid[6];
  uint32_t id = 0;
  config.loadConfig();
  int level=config.getInt(config.logLevel,LOGLEVEL);
  logger.setLevel(level);
  bool fallbackSerial=false;
#ifdef FALLBACK_SERIAL
  fallbackSerial=true;
    //falling back to old style serial for logging
    USBSerial.begin(115200);
    USBSerial.printf("fallback serial enabled\n");
    logger.prefix="FALLBACK:";
  logger.setWriter(new DefaultLogWriter());
#endif
  boatData.begin();
  userCodeHandler.begin(mainLock);
  userCodeHandler.startInitTasks(MIN_USER_TASK);
  channels.preinit();
  config.stopChanges();
  //maybe the user code changed the level
  level=config.getInt(config.logLevel,LOGLEVEL);
  logger.setLevel(level);
  sendOutN2k=config.getBool(config.sendN2k,true);
  logger.logDebug(GwLog::LOG,"send N2k=%s",(sendOutN2k?"true":"false"));
  gwWifi.setup();
  MDNS.begin(config.getConfigItem(config.systemName)->asCString());
  channels.begin(fallbackSerial);
  logger.flush();
  config.logConfig(GwLog::DEBUG);
  webserver.registerMainHandler("/api/reset", [](AsyncWebServerRequest *request)->GwRequestMessage *{
    return new ResetRequest(request->arg("_hash"));
  });
  webserver.registerMainHandler("/api/capabilities", [](AsyncWebServerRequest *request)->GwRequestMessage *{
    return new CapabilitiesRequest();
  });
  webserver.registerMainHandler("/api/converterInfo", [](AsyncWebServerRequest *request)->GwRequestMessage *{
    return new ConverterInfoRequest();
  });
  webserver.registerMainHandler("/api/status", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { return new StatusRequest(); });
  webserver.registerMainHandler("/api/config", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { return new ConfigRequest(); });
  webserver.registerMainHandler("/api/resetConfig", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { return new ResetConfigRequest(request->arg("_hash")); });
  webserver.registerMainHandler("/api/boatData", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { return new BoatDataRequest(); });
  webserver.registerMainHandler("/api/boatDataString", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { return new BoatDataStringRequest(); });                              
  webserver.registerMainHandler("/api/xdrExample", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { 
                                String mapping=request->arg("mapping");
                                double value=atof(request->arg("value").c_str());
                                return new XdrExampleRequest(mapping,value); 
                              });
  webserver.registerMainHandler("/api/xdrUnmapped", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { return new XdrUnMappedRequest(); });                              
  webserver.registerMainHandler("/api/checkPass", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { 
                                String hash=request->arg("hash");
                                return new CheckPassRequest(hash); 
                              });
  webserver.registerPostHandler("/api/setConfig",
                              [](AsyncWebServerRequest *request){

                              },
                              handleConfigRequestData);
  webserver.registerHandler("/api/calibrate",[](AsyncWebServerRequest *request){
                              const String name=request->arg("name");
                              double value;
                              if (! apiImpl->getCalibrationValue(name,value)){
                                request->send(400, "text/plain", "name not found");
                                return;
                              }
                              char buffer[30];
                              snprintf(buffer,29,"%g",value);
                              buffer[29]=0;
                              request->send(200,"text/plain",buffer);    
  });
  webserver.registerHandler((USERPREFIX+"*").c_str(),[](AsyncWebServerRequest *req){
                              String turl=req->url().substring(USERPREFIX.length());
                              logger.logDebug(GwLog::DEBUG,"user web request for %s",turl.c_str());
                              userCodeHandler.handleWebRequest(turl,req);
  });

  webserver.begin();
  xdrMappings.begin();
  logger.flush();
  GwConverterConfig converterConfig;
  converterConfig.init(&config,&logger);
  nmea0183Converter= N2kDataToNMEA0183::create(&logger, &boatData, 
    [](const tNMEA0183Msg &msg, int sourceId){
      SendNMEA0183Message(msg,sourceId,false);
    }
    , 
    config.getString(config.talkerId,String("GP")),
    &xdrMappings,
    converterConfig
    );

  toN2KConverter= NMEA0183DataToN2K::create(&logger,&boatData,[](const tN2kMsg &msg, int sourceId)->bool{
    logger.logDebug(GwLog::DEBUG+2,"send N2K %ld",msg.PGN);
    handleN2kMessage(msg,sourceId,true);
    return true;
  },
  &xdrMappings,
  converterConfig
  );  
  
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  esp_efuse_mac_get_default(chipid);
  for (int i = 0; i < 6; i++) id += (chipid[i] << (7 * i));

  // Set product information
  NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 systemName->asCString(),  // Manufacturer's Model ID
                                 FIRMWARE_TYPE,  // Manufacturer's Software version code
                                 VERSION, // Manufacturer's Model version,
                                 N2K_LOAD_LEVEL,
                                 0xffff, //Version
                                 N2K_CERTIFICATION_LEVEL
                                );
  // Set device information
  NMEA2000.SetDeviceInformation(id, // Unique number. Use e.g. Serial number. Id is generated from MAC-Address
                                130, // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );

  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below

  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.

  preferences.begin("nvs", false);                          // Open nonvolatile storage (nvs)
  NodeAddress = preferences.getInt("LastNodeAddress", 32);  // Read stored last NodeAddress, default 32
  preferences.end();

  logger.logDebug(GwLog::LOG,"NodeAddress=%d", NodeAddress);
  logger.flush();
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, NodeAddress);
  NMEA2000.SetForwardOwnMessages(false);
  NMEA2000.SetHeartbeatIntervalAndOffset(NMEA2000_HEARTBEAT_INTERVAL);
  if (sendOutN2k){
    // Set the information for other bus devices, which messages we support
    unsigned long *pgns=toN2KConverter->handledPgns();
    if (logger.isActive(GwLog::DEBUG)){
      unsigned long *op=pgns;
      while (*op != 0){
        logger.logDebug(GwLog::DEBUG,"add transmit pgn %ld",(long)(*op));
        logger.flush();
        op++;
      }
    }
    NMEA2000.ExtendTransmitMessages(pgns);
  }
  NMEA2000.ExtendReceiveMessages(nmea0183Converter->handledPgns());
  NMEA2000.SetMsgHandler([](const tN2kMsg &n2kMsg){
    handleN2kMessage(n2kMsg,N2K_CHANNEL_ID);
  });
  NMEA2000.Open();
  logger.logDebug(GwLog::LOG,"starting addon tasks");
  logger.flush();
  {
    GWSYNCHRONIZED(mainLock);
    userCodeHandler.startUserTasks(MIN_USER_TASK);
  }
  timers.addAction(HEAP_REPORT_TIME,[](){
    if (logger.isActive(GwLog::DEBUG)){
      logger.logDebug(GwLog::DEBUG,"Heap free=%ld, minFree=%ld",
          (long)xPortGetFreeHeapSize(),
          (long)xPortGetMinimumEverFreeHeapSize()
      );
      logger.logDebug(GwLog::DEBUG,"Main loop %s",monitor.getLog().c_str());
    }
  });
  logger.logString("wifi AP pass: %s",fixedApPass? gwWifi.AP_password:config.getString(config.apPassword).c_str());
  logger.logString("admin pass: %s",config.getString(config.adminPassword).c_str());
  logger.logDebug(GwLog::LOG,"setup done");
  #ifdef OWN_LOOP
  logger.logDebug(GwLog::LOG,"starting own main loop");
  xTaskCreateUniversal(loopFunction,"loop",8192,NULL,1,NULL,ARDUINO_RUNNING_CORE);
  #endif
}  
//*****************************************************************************
void handleSendAndRead(bool handleRead){
  channels.allChannels([&](GwChannel *c){
    c->loop(handleRead,true);
  });
}

void loopRun() {
  //logger.logDebug(GwLog::DEBUG,"main loop start");
  monitor.reset();
  GWSYNCHRONIZED(mainLock);
  logger.flush();
  monitor.setTime(1);
  gwWifi.loop();
  unsigned long now=millis();
  monitor.setTime(2);
  timers.loop();
  monitor.setTime(3);
  NMEA2000.loop();
  monitor.setTime(4);
  channels.allChannels([](GwChannel *c){
    c->loop(true,false);
  });
  //reads
  monitor.setTime(5);
  channels.allChannels([](GwChannel *c){
    c->loop(false,true);
  });
  //writes
  monitor.setTime(6);  
  NMEA2000.ParseMessages();
  monitor.setTime(7);

  int SourceAddress = NMEA2000.GetN2kSource();
  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    NodeAddress = SourceAddress;      // Set new Node Address (to save only once)
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    logger.logDebug(GwLog::LOG,"Address Change: New Address=%d\n", SourceAddress);
  }
  //potentially send out an own RMC if we did not receive one
  nmea0183Converter->loop(toN2KConverter->getLastRmc());
  monitor.setTime(8);

  //read channels
  channels.allChannels([](GwChannel *c){
    c->readMessages([&](const char * buffer, int sourceId){
      bool isSeasmart=false;
      if (strlen(buffer) > 6 && strncmp(buffer,"$PCDIN",6) == 0){
        isSeasmart=true;
      }
      channels.allChannels([&](GwChannel *oc){
        oc->sendToClients(buffer,sourceId,isSeasmart);
        oc->loop(false,true);
      });
      if (c->sendToN2K()){
        if (isSeasmart){
          tN2kMsg n2kMsg;
          uint32_t timestamp;
          if (SeasmartToN2k(buffer,timestamp,n2kMsg)){
            handleN2kMessage(n2kMsg,sourceId);
          }
        }
        else{
          toN2KConverter->parseAndSend(buffer, sourceId);
        }
      }
    });
  });
  monitor.setTime(9);
  channels.allChannels([](GwChannel *c){
    c->parseActisense([](const tN2kMsg &msg,int source){
      handleN2kMessage(msg,source);
    });
  });
  monitor.setTime(10);

  //handle message requests
  GwMessage *msg=mainQueue.fetchMessage(0);
  if (msg){
    msg->process();
    msg->unref();
  }
  monitor.setTime(11);
  //logger.logDebug(GwLog::DEBUG,"main loop end");
}

