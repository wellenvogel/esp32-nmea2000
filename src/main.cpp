/*
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

#define GWSTR(x) #x
#define GWSTRINGIFY(x) GWSTR(x)
#ifdef GWRELEASEVERSION
#define VERSION GWSTRINGIFY(GWRELEASEVERSION)
#define LOGLEVEL GwLog::ERROR
#endif
#ifdef GWDEVVERSION
#define VERSION GWSTRINGIFY(GWDEVVERSION)
#define LOGLEVEL GwLog::DEBUG
#endif
#ifndef VERSION
#define VERSION "0.7.0"
#define LOGLEVEL GwLog::DEBUG
#endif

// #define GW_MESSAGE_DEBUG_ENABLED
//#define FALLBACK_SERIAL
const unsigned long HEAP_REPORT_TIME=2000; //set to 0 to disable heap reporting
#include <Arduino.h>
#include "GwApi.h"
#include "GwHardware.h"

#ifndef N2K_LOAD_LEVEL
#define N2K_LOAD_LEVEL 0
#endif

#ifndef N2K_CERTIFICATION_LEVEL
#define N2K_CERTIFICATION_LEVEL 0xff
#endif

#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <ActisenseReader.h>
#include <Seasmart.h>
#include <N2kMessages.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <map>
#include <vector>
#include "esp_heap_caps.h"
#include <MD5Builder.h>
#include "GwJsonDocument.h"
#include "N2kDataToNMEA0183.h"


#include "GwLog.h"
#include "GWConfig.h"
#include "GWWifi.h"
#include "GwSocketServer.h"
#include "GwBoatData.h"
#include "GwMessage.h"
#include "GwSerial.h"
#include "GwWebServer.h"
#include "NMEA0183DataToN2K.h"
#include "GwButtons.h"
#include "GwLeds.h"
#include "GwCounter.h"
#include "GwXDRMappings.h"
#include "GwSynchronized.h"
#include "GwUserCode.h"
#include "GwStatistics.h"
#include "GwUpdate.h"

//NMEA message channels
#define N2K_CHANNEL_ID 0
#define USB_CHANNEL_ID 1
#define SERIAL1_CHANNEL_ID 2
#define MIN_TCP_CHANNEL_ID 3

#define MIN_USER_TASK 200

#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500
#define MAX_NMEA0183_MESSAGE_SIZE 150 // For AIS



typedef std::map<String,String> StringMap;


GwLog logger(LOGLEVEL,NULL);
GwConfigHandler config(&logger);
#ifdef GWBUTTON_PIN
bool fixedApPass=false;
#else
bool fixedApPass=true;
#endif
GwWifi gwWifi(&config,&logger,fixedApPass);
GwSocketServer socketServer(&config,&logger,MIN_TCP_CHANNEL_ID);
GwBoatData boatData(&logger);
GwXDRMappings xdrMappings(&logger,&config);


int NodeAddress;  // To store last Node Address

Preferences preferences;             // Nonvolatile storage on ESP32 - To store LastDeviceAddress
N2kDataToNMEA0183 *nmea0183Converter=NULL;
NMEA0183DataToN2K *toN2KConverter=NULL;
tActisenseReader *actisenseReader=NULL;
Stream *usbStream=NULL;
SemaphoreHandle_t mainLock;


GwRequestQueue mainQueue(&logger,20);
GwWebServer webserver(&logger,&mainQueue,80);

GwCounter<unsigned long> countNMEA2KIn("count2Kin");
GwCounter<unsigned long> countNMEA2KOut("count2Kout");

GwCounter<String> countUSBIn("countUSBin");
GwCounter<String> countUSBOut("countUSBout");
GwCounter<String> countTCPIn("countTCPin");
GwCounter<String> countTCPOut("countTCPout");
GwCounter<String> countSerialIn("countSerialIn");
GwCounter<String> countSerialOut("countSerialOut");

unsigned long saltBase=esp_random();

char hv(uint8_t nibble){
  nibble=nibble&0xf;
  if (nibble < 10) return (char)('0'+nibble);
  return (char)('A'+nibble-10);
}
void toHex(unsigned long v,char *buffer,size_t bsize){
  uint8_t *bp=(uint8_t *)&v;
  size_t i=0;
  for (;i<sizeof(v) && (2*i +1)< bsize;i++){
    buffer[2*i]=hv((*bp) >> 4);
    buffer[2*i+1]=hv(*bp);
    bp++;
  }
  if ((2*i) < bsize) buffer[2*i]=0;
}

bool checkPass(String hash){
  if (! config.getBool(config.useAdminPass)) return true;
  String pass=config.getString(config.adminPassword);
  unsigned long now=millis()/1000UL & ~0x7UL;
  MD5Builder builder;
  char buffer[2*sizeof(now)+1];
  for (int i=0;i< 5 ;i++){
    unsigned long base=saltBase+now;
    toHex(base,buffer,2*sizeof(now)+1);
    builder.begin();
    builder.add(buffer);
    builder.add(pass);
    builder.calculate();
    String md5=builder.toString();
    bool rt=hash == md5;
    logger.logDebug(GwLog::DEBUG,"checking pass %s, base=%ld, hash=%s, res=%d",
      hash.c_str(),base,md5.c_str(),(int)rt);
    if (rt) return true;
    now -= 8;
  }
  return false;
}

GwUpdate updater(&logger,&webserver,&checkPass);

void updateNMEACounter(int id,const char *msg,bool incoming,bool fail=false){
  //we rely on the msg being long enough
  char key[6];
  if (msg[0] == '$') {
    strncpy(key,&msg[3],3);
    key[3]=0;
  }
  else if(msg[0] == '!'){
    strncpy(key,&msg[1],5);
    key[5]=0;
  }
  else return;
  GwCounter<String> *counter=NULL;
  if (id == USB_CHANNEL_ID) counter=incoming?&countUSBIn:&countUSBOut;
  if (id == SERIAL1_CHANNEL_ID) counter=incoming?&countSerialIn:&countSerialOut;
  if (id >= MIN_TCP_CHANNEL_ID) counter=incoming?&countTCPIn:&countTCPOut;
  if (! counter) return;
  if (fail){
    counter->addFail(key);
  }
  else{
    counter->add(key);
  }
}

//configs that we need in main


GwConfigInterface *sendUsb=config.getConfigItem(config.sendUsb,true);
GwConfigInterface *readUsb=config.getConfigItem(config.receiveUsb,true);
GwConfigInterface *usbActisense=config.getConfigItem(config.usbActisense,true);
GwConfigInterface *usbSendActisens=config.getConfigItem(config.usbActSend,true);
GwConfigInterface *sendTCP=config.getConfigItem(config.sendTCP,true);
GwConfigInterface *readTCP=config.getConfigItem(config.readTCP,true);
GwConfigInterface *sendSeasmart=config.getConfigItem(config.sendSeasmart,true);
GwConfigInterface *systemName=config.getConfigItem(config.systemName,true);
GwConfigInterface *n2kFromTCP=config.getConfigItem(config.tcpToN2k,true);
GwConfigInterface *n2kFromUSB=config.getConfigItem(config.usbToN2k,true);
GwConfigInterface *receiveSerial=config.getConfigItem(config.receiveSerial,true);
GwConfigInterface *sendSerial=config.getConfigItem(config.sendSerial,true);
GwConfigInterface *n2kFromSerial=config.getConfigItem(config.serialToN2k,true);
GwNmeaFilter usbReadFilter(config.getConfigItem(config.usbReadFilter,true));
GwNmeaFilter usbWriteFilter(config.getConfigItem(config.usbWriteFilter,true));
GwNmeaFilter serialReadFilter(config.getConfigItem(config.serialReadF,true));
GwNmeaFilter serialWriteFilter(config.getConfigItem(config.serialWriteF,true));
GwNmeaFilter tcpReadFilter(config.getConfigItem(config.tcpReadFilter,true));
GwNmeaFilter tcpWriteFilter(config.getConfigItem(config.tcpWriteFilter,true));

bool checkFilter(const char *buffer,int channelId,bool read){
  GwNmeaFilter *filter=NULL;
  if (channelId == USB_CHANNEL_ID) filter=read?&usbReadFilter:&usbWriteFilter;
  else if (channelId == SERIAL1_CHANNEL_ID) filter=read?&serialReadFilter:&serialWriteFilter;
  else if (channelId >= MIN_TCP_CHANNEL_ID) filter=read?&tcpReadFilter:&tcpWriteFilter;
  if (!filter) return true;
  if (filter->canPass(buffer)) return true;
  logger.logDebug(GwLog::DEBUG,"%s filter for channel %d dropped %s",(read?"read":"write"),channelId,buffer);
  return false;
}

bool serCanWrite=true;
bool serCanRead=true;

GwSerial *usbSerial = new GwSerial(NULL, 0, USB_CHANNEL_ID);
GwSerial *serial1=NULL;

void sendBufferToChannels(const char * buffer, int sourceId){
  if (sendTCP->asBoolean() && checkFilter(buffer,MIN_TCP_CHANNEL_ID,false)){
    socketServer.sendToClients(buffer,sourceId);
    updateNMEACounter(MIN_TCP_CHANNEL_ID,buffer,false);
  }
  if (! actisenseReader && sendUsb->asBoolean() && checkFilter(buffer,USB_CHANNEL_ID,false)){
    usbSerial->sendToClients(buffer,sourceId);
    updateNMEACounter(USB_CHANNEL_ID,buffer,false);
  }
  if (serial1 && serCanWrite && checkFilter(buffer,SERIAL1_CHANNEL_ID,false)){
    serial1->sendToClients(buffer,sourceId);
    updateNMEACounter(SERIAL1_CHANNEL_ID,buffer,false);
  }
}
typedef enum {
  N2KT_MSGIN,  //from CAN
  N2KT_MSGINT, //from internal source
  N2KT_MSGOUT, //from converter
  N2KT_MSGACT  //from actisense
} N2K_MsgDirection;
void handleN2kMessage(const tN2kMsg &n2kMsg,N2K_MsgDirection direction)
{
  logger.logDebug(GwLog::DEBUG + 1, "N2K: pgn %d, dir %d", 
    n2kMsg.PGN,(int)direction);
  if (direction == N2KT_MSGIN){
    countNMEA2KIn.add(n2kMsg.PGN);
  }
  if (sendSeasmart->asBoolean())
  {
    char buf[MAX_NMEA2000_MESSAGE_SEASMART_SIZE];
    if (N2kToSeasmart(n2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE) == 0)
      return;
    socketServer.sendToClients(buf, N2K_CHANNEL_ID);
  }
  if (actisenseReader && direction != N2KT_MSGACT && usbStream && usbSendActisens->asBoolean())
  {
    countUSBOut.add(String(n2kMsg.PGN));
    n2kMsg.SendInActisenseFormat(usbStream);
  }
  if (direction != N2KT_MSGOUT){
    nmea0183Converter->HandleMsg(n2kMsg);
  }
  if (direction != N2KT_MSGIN){
    countNMEA2KOut.add(n2kMsg.PGN);
    NMEA2000.SendMsg(n2kMsg);
  }
};

void handleReceivedNmeaMessage(const char *buf, int sourceId){
  if (! checkFilter(buf,sourceId,true)) return;
  updateNMEACounter(sourceId,buf,true);
  if ( (sourceId >= MIN_USER_TASK) ||
      (sourceId == USB_CHANNEL_ID && n2kFromUSB->asBoolean())||
      (sourceId >= MIN_TCP_CHANNEL_ID && n2kFromTCP->asBoolean())||
      (sourceId == SERIAL1_CHANNEL_ID && n2kFromSerial->asBoolean())
    )
    toN2KConverter->parseAndSend(buf,sourceId);
  sendBufferToChannels(buf,sourceId);
}  

//*****************************************************************************
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg, int sourceId,bool convert=false) {
  logger.logDebug(GwLog::DEBUG+2,"SendNMEA0183(1)");
  char buf[MAX_NMEA0183_MESSAGE_SIZE+3];
  if ( !NMEA0183Msg.GetMessage(buf, MAX_NMEA0183_MESSAGE_SIZE) ) return;
  logger.logDebug(GwLog::DEBUG+2,"SendNMEA0183: %s",buf);
  if (convert){
    toN2KConverter->parseAndSend(buf,sourceId);
  }
  size_t len=strlen(buf);
  buf[len]=0x0d;
  buf[len+1]=0x0a;
  buf[len+2]=0;
  sendBufferToChannels(buf,sourceId);
}

class GwSerialLog : public GwLogWriter{
  static const size_t bufferSize=4096;
  char *logBuffer=NULL;
  int wp=0;
  public:
    GwSerialLog(){
      logBuffer=new char[bufferSize];
      wp=0;
    }
    virtual ~GwSerialLog(){}
    virtual void write(const char *data){
      int len=strlen(data);
      if ((wp+len) >= (bufferSize-1)) return;
      strncpy(logBuffer+wp,data,len);
      wp+=len;
      logBuffer[wp]=0;
    }
    virtual void flush(){
      size_t handled=0;
      while (handled < wp){
        usbSerial->flush();
        size_t rt=usbSerial->sendToClients(logBuffer+handled,-1,true);
        handled+=rt;
        }
      wp=0;
      logBuffer[0]=0;
    }

};

GwSerialLog logWriter;


class ApiImpl : public GwApi
{
private:
  int sourceId = -1;

public:
  ApiImpl(int sourceId)
  {
    this->sourceId = sourceId;
  }
  virtual GwRequestQueue *getQueue()
  {
    return &mainQueue;
  }
  virtual void sendN2kMessage(const tN2kMsg &msg,bool convert)
  {
    handleN2kMessage(msg,convert?N2KT_MSGINT:N2KT_MSGOUT);
    
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
      if (item){
        list[i]->valid=item->isValid();
        if (list[i]->valid) list[i]->value=item->getDoubleValue();
        list[i]->setFormat(item->getFormat());
      }
      else{
        list[i]->valid=false;
      }
    }
  }
  virtual GwBoatData *getBoatData(){
    return &boatData;
  }
  virtual const char* getTalkerId(){
    return config.getString(config.talkerId,String("GP")).c_str();
  }
  virtual ~ApiImpl(){}
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
GwUserCode userCodeHandler(apiImpl,&mainLock);

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
    GwJsonDocument status(256 + 
      countNMEA2KIn.getJsonSize()+
      countNMEA2KOut.getJsonSize() +
      countUSBIn.getJsonSize()+
      countUSBOut.getJsonSize()+
      countSerialIn.getJsonSize()+
      countSerialOut.getJsonSize()+
      countTCPIn.getJsonSize()+
      countTCPOut.getJsonSize()
      );
    status["version"] = VERSION;
    status["wifiConnected"] = gwWifi.clientConnected();
    status["clientIP"] = WiFi.localIP().toString();
    status["numClients"] = socketServer.numClients();
    status["apIp"] = gwWifi.apIP();
    size_t bsize=2*sizeof(unsigned long)+1;
    unsigned long base=saltBase + ( millis()/1000UL & ~0x7UL);
    char buffer[bsize];
    toHex(base,buffer,bsize);
    status["salt"] = buffer;
    //nmea0183Converter->toJson(status);
    countNMEA2KIn.toJson(status);
    countNMEA2KOut.toJson(status);
    countUSBIn.toJson(status);
    countUSBOut.toJson(status);
    countSerialIn.toJson(status);
    countSerialOut.toJson(status);
    countTCPIn.toJson(status);
    countTCPOut.toJson(status);
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
      GwJsonDocument json(JSON_OBJECT_SIZE(numCapabilities*3+6));
      for (auto it=userCodeHandler.getCapabilities()->begin();
        it != userCodeHandler.getCapabilities()->end();it++){
          json[it->first]=it->second;
        }
      #ifdef GWSERIAL_MODE
      String serial(F(GWSERIAL_MODE));
      #else
      String serial(F("NONE"));
      #endif
      json["serialmode"]=serial;
      #ifdef GWBUTTON_PIN
      json["hardwareReset"]="true";
      #endif
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

class SetConfigRequest : public GwRequestMessage
{
public:
  SetConfigRequest() : GwRequestMessage(F("application/json"),F("setConfig")){};
  StringMap args;

protected:
  virtual void processRequest()
  {
    bool ok = true;
    String error;
    String hash;
    auto it=args.find("_hash");
    if (it != args.end()){
      hash=it->second;
    }
    if (! checkPass(hash)){
      result=JSON_INVALID_PASS;
      return;
    }
    for (StringMap::iterator it = args.begin(); it != args.end(); it++)
    {
      if (it->first.indexOf("_")>= 0) continue;
      bool rt = config.updateValue(it->first, it->second);
      if (!rt)
      {
        logger.logString("ERR: unable to update %s to %s", it->first.c_str(), it->second.c_str());
        ok = false;
        error += it->first;
        error += "=";
        error += it->second;
        error += ",";
      }
    }
    if (ok)
    {
      result = JSON_OK;
      logger.logString("update config and restart");
      config.saveConfig();
      delayedRestart();
    }
    else
    {
      GwJsonDocument rt(100);
      rt["status"] = error;
      serializeJson(rt, result);
    }
  }
};
class ResetConfigRequest : public GwRequestMessage
{
  String hash;
public:
  ResetConfigRequest(String hash) : GwRequestMessage(F("application/json"),F("resetConfig")){
    this->hash=hash;
  };

protected:
  virtual void processRequest()
  {
    if (! checkPass(hash)){
      result=JSON_INVALID_PASS;
      return;
    }
    config.reset(true);
    logger.logString("reset config, restart");
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
    char buf[MAX_NMEA0183_MSG_BUF_LEN];
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



void setup() {
  mainLock=xSemaphoreCreateMutex();
  uint8_t chipid[6];
  uint32_t id = 0;
  config.loadConfig();
  // Init USB serial port
  GwConfigInterface *usbBaud=config.getConfigItem(config.usbBaud,false);
  int baud=115200;
  if (usbBaud){
    baud=usbBaud->asInt();
  }
#ifdef FALLBACK_SERIAL
  int st=-1;
#else
  int st=usbSerial->setup(baud,3,1); //TODO: PIN defines  
#endif  
  if (st < 0){
    //falling back to old style serial for logging
    Serial.begin(baud);
    Serial.printf("fallback serial enabled, error was %d\n",st);
    logger.prefix="FALLBACK:";
  }
  else{
    logger.prefix="GWSERIAL:";
    logger.setWriter(&logWriter);
    logger.logDebug(GwLog::LOG,"created GwSerial for USB port");
  }  
  logger.logDebug(GwLog::LOG,"config: %s", config.toString().c_str());
  userCodeHandler.startInitTasks(MIN_USER_TASK);
  #ifdef GWSERIAL_MODE
  int serialrx=-1;
  int serialtx=-1;
  #ifdef GWSERIAL_TX
  serialtx=GWSERIAL_TX;
  #endif
  #ifdef GWSERIAL_RX
  serialrx=GWSERIAL_RX;
  #endif
  //the mode is a compile time preselection from hardware.h
  String serialMode(F(GWSERIAL_MODE));
  //the serial direction is from the config (only valid for mode UNI)
  String serialDirection=config.getString(config.serialDirection);
  //we only consider the direction if mode is UNI
  if (serialMode != String("UNI")){
    serialDirection=String("");
    //if mode is UNI it depends on the selection
    serCanRead=receiveSerial->asBoolean();
    serCanWrite=sendSerial->asBoolean();
  }
  if (serialDirection == "receive" || serialDirection == "off" || serialMode == "RX") serCanWrite=false;
  if (serialDirection == "send" || serialDirection == "off" || serialMode == "TX") serCanRead=false;
  logger.logDebug(GwLog::DEBUG,"serial set up: mode=%s,direction=%s,rx=%d,tx=%d",
    serialMode.c_str(),serialDirection.c_str(),serialrx,serialtx
  );
  if (serialtx != -1 || serialrx != -1){
    logger.logDebug(GwLog::LOG,"creating serial interface rx=%d, tx=%d",serialrx,serialtx);
    serial1=new GwSerial(&logger,1,SERIAL1_CHANNEL_ID,serCanRead);
  }
  if (serial1){
    int rt=serial1->setup(config.getInt(config.serialBaud,115200),serialrx,serialtx);
    logger.logDebug(GwLog::LOG,"starting serial returns %d",rt);
  }
  #endif
  
  MDNS.begin(config.getConfigItem(config.systemName)->asCString());
  gwWifi.setup();

  // Start TCP server
  socketServer.begin();
  logger.flush();

  logger.logDebug(GwLog::LOG,"usbRead: %s", usbReadFilter.toString().c_str());
  logger.flush();

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
  webserver.registerMainHandler("/api/setConfig",
                              [](AsyncWebServerRequest *request)->GwRequestMessage *
                              {
                                StringMap args;
                                for (int i = 0; i < request->args(); i++)
                                {
                                  args[request->argName(i)] = request->arg(i);
                                }
                                SetConfigRequest *msg = new SetConfigRequest();
                                msg->args = args;
                                return msg;
                              });
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

  webserver.begin();
  xdrMappings.begin();
  logger.flush();
  
  nmea0183Converter= N2kDataToNMEA0183::create(&logger, &boatData, 
    [](const tNMEA0183Msg &msg, int sourceId){
      SendNMEA0183Message(msg,sourceId,false);
    }
    , N2K_CHANNEL_ID,
    config.getString(config.talkerId,String("GP")),
    &xdrMappings,
    config.getInt(config.minXdrInterval,100)
    );

  toN2KConverter= NMEA0183DataToN2K::create(&logger,&boatData,[](const tN2kMsg &msg)->bool{
    logger.logDebug(GwLog::DEBUG+2,"send N2K %ld",msg.PGN);
    handleN2kMessage(msg,N2KT_MSGOUT);
    return true;
  },
  &xdrMappings,
  config.getInt(config.min2KInterval,50)
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
                                 VERSION,  // Manufacturer's Software version code
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
  NMEA2000.ExtendReceiveMessages(nmea0183Converter->handledPgns());
  if (usbActisense->asBoolean()){
    actisenseReader=new tActisenseReader();
    usbStream=usbSerial->getStream(false);
    actisenseReader->SetReadStream(usbStream);
    actisenseReader->SetMsgHandler([](const tN2kMsg &msg){
      countUSBIn.add(String(msg.PGN));
      handleN2kMessage(msg,N2KT_MSGACT);
    });
  } 
  NMEA2000.SetMsgHandler([](const tN2kMsg &n2kMsg){
    handleN2kMessage(n2kMsg,N2KT_MSGIN);
  });
  NMEA2000.Open();
  logger.logDebug(GwLog::LOG,"starting addon tasks");
  logger.flush();
  userCodeHandler.startAddonTask(F("handleButtons"),handleButtons,100);
  setLedMode(LED_GREEN);
  userCodeHandler.startAddonTask(F("handleLeds"),handleLeds,101);
  {
    GWSYNCHRONIZED(&mainLock);
    userCodeHandler.startUserTasks(MIN_USER_TASK);
  }
  logger.logDebug(GwLog::ERROR,"wifi AP pass: %s",config.getString(config.apPassword).c_str());
  logger.logDebug(GwLog::ERROR,"admin pass: %s",config.getString(config.adminPassword).c_str());
  logger.logDebug(GwLog::LOG,"setup done");
}  
//*****************************************************************************
void handleSendAndRead(bool handleRead){
  socketServer.loop(handleRead);  
  usbSerial->loop(handleRead);
  if (serial1) serial1->loop(handleRead);
}
class NMEAMessageReceiver : public GwMessageFetcher{
  static const int bufferSize=GwBuffer::RX_BUFFER_SIZE+4;
  uint8_t buffer[bufferSize];
  uint8_t *writePointer=buffer;
  public:
    virtual bool handleBuffer(GwBuffer *gwbuffer){
      size_t len=fetchMessageToBuffer(gwbuffer,buffer,bufferSize-4,'\n');
      writePointer=buffer+len;
      if (writePointer == buffer) return false;
      uint8_t *p;
      for (p=writePointer-1;p>=buffer && *p <= 0x20;p--){
        *p=0;
      }
      if (p > buffer){
        p++;
        *p=0x0d;
        p++;
        *p=0x0a;
        p++;
        *p=0;
      }
      for (p=buffer; *p != 0 && p < writePointer && *p <= 0x20;p++){}
      //very simple NMEA check
      if (*p != '!' && *p != '$'){
        logger.logDebug(GwLog::DEBUG,"unknown line [%d] - ignore: %s",id,(const char *)p);  
      }
      else{
        logger.logDebug(GwLog::DEBUG,"NMEA[%d]: %s",id,(const char *)p);
        handleReceivedNmeaMessage((const char *)p,id);
        //trigger sending to empty buffers
        handleSendAndRead(false);
      }
      writePointer=buffer;
      return true;
    }
};

TimeMonitor monitor(20,0.2);
NMEAMessageReceiver receiver;
unsigned long lastHeapReport=0;
void loop() {
  monitor.reset();
  GWSYNCHRONIZED(&mainLock);
  logger.flush();
  monitor.setTime(1);
  gwWifi.loop();
  unsigned long now=millis();
  monitor.setTime(2);
  if (HEAP_REPORT_TIME > 0 && now > (lastHeapReport+HEAP_REPORT_TIME)){
    lastHeapReport=now;
    if (logger.isActive(GwLog::DEBUG)){
      logger.logDebug(GwLog::DEBUG,"Heap free=%ld, minFree=%ld",
          (long)xPortGetFreeHeapSize(),
          (long)xPortGetMinimumEverFreeHeapSize()
      );
      logger.logDebug(GwLog::DEBUG,"Main loop %s",monitor.getLog().c_str());
    }
  }
  monitor.setTime(3);
  //read sockets
  socketServer.loop(true,false);
  monitor.setTime(4);
  //write sockets
  socketServer.loop(false,true);
  monitor.setTime(5);  
  usbSerial->loop(true);
  monitor.setTime(6);
  if (serial1) serial1->loop(true);
  monitor.setTime(7);
  handleSendAndRead(true);
  monitor.setTime(8);
  NMEA2000.ParseMessages();
  monitor.setTime(9);

  int SourceAddress = NMEA2000.GetN2kSource();
  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    NodeAddress = SourceAddress;      // Set new Node Address (to save only once)
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    logger.logDebug(GwLog::LOG,"Address Change: New Address=%d\n", SourceAddress);
  }
  nmea0183Converter->loop();
  monitor.setTime(10);

  //read channels
  if (readTCP->asBoolean()) socketServer.readMessages(&receiver);
  monitor.setTime(11);
  receiver.id=USB_CHANNEL_ID;
  if (! actisenseReader && readUsb->asBoolean()) usbSerial->readMessages(&receiver);
  monitor.setTime(12);
  receiver.id=SERIAL1_CHANNEL_ID;
  if (serial1 && serCanRead ) serial1->readMessages(&receiver);
  monitor.setTime(13);
  if (actisenseReader){
    actisenseReader->ParseMessages();
  }
  monitor.setTime(14);

  //handle message requests
  GwMessage *msg=mainQueue.fetchMessage(0);
  if (msg){
    msg->process();
    msg->unref();
  }
  monitor.setTime(15);
}
