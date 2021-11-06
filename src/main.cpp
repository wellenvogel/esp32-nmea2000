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

#define VERSION "0.6.1"

// #define GW_MESSAGE_DEBUG_ENABLED
// #define FALLBACK_SERIAL
const unsigned long HEAP_REPORT_TIME=2000; //set to 0 to disable heap reporting
#include "GwHardware.h"

#include <Arduino.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <Seasmart.h>
#include <N2kMessages.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <map>
#include "esp_heap_caps.h"

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
#include "GwApi.h"
#include "GwButtons.h"
#include "GwLeds.h"


//NMEA message channels
#define N2K_CHANNEL_ID 0
#define USB_CHANNEL_ID 1
#define SERIAL1_CHANNEL_ID 2
#define MIN_TCP_CHANNEL_ID 3

#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500
#define MAX_NMEA0183_MESSAGE_SIZE 150 // For AIS

typedef std::map<String,String> StringMap;


GwLog logger(GwLog::DEBUG,NULL);
GwConfigHandler config(&logger);
#ifdef GWBUTTON_PIN
bool fixedApPass=false;
#else
bool fixedApPass=true;
#endif
GwWifi gwWifi(&config,&logger,fixedApPass);
GwSocketServer socketServer(&config,&logger,MIN_TCP_CHANNEL_ID);
GwBoatData boatData(&logger);


//counter
int numCan=0;


int NodeAddress;  // To store last Node Address

Preferences preferences;             // Nonvolatile storage on ESP32 - To store LastDeviceAddress
N2kDataToNMEA0183 *nmea0183Converter=NULL;
NMEA0183DataToN2K *toN2KConverter=NULL;


void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg,int id);

GwRequestQueue mainQueue(&logger,20);
GwWebServer webserver(&logger,&mainQueue,80);

//configs that we need in main


GwConfigInterface *sendUsb=config.getConfigItem(config.sendUsb,true);
GwConfigInterface *sendTCP=config.getConfigItem(config.sendTCP,true);
GwConfigInterface *sendSeasmart=config.getConfigItem(config.sendSeasmart,true);
GwConfigInterface *systemName=config.getConfigItem(config.systemName,true);
GwConfigInterface *n2kFromTCP=config.getConfigItem(config.tcpToN2k,true);
GwConfigInterface *n2kFromUSB=config.getConfigItem(config.usbToN2k,true);
GwConfigInterface *receiveSerial=config.getConfigItem(config.receiveSerial,true);
GwConfigInterface *sendSerial=config.getConfigItem(config.sendSerial,true);
GwConfigInterface *n2kFromSerial=config.getConfigItem(config.serialToN2k,true);
GwNmeaFilter usbReadFilter(config.getConfigItem(config.usbReadFilter,true));
GwNmeaFilter usbWriteFilter(config.getConfigItem(config.usbWriteFilter,true));
GwNmeaFilter serialReadFilter(config.getConfigItem(config.serialReadFilter,true));
GwNmeaFilter serialWriteFilter(config.getConfigItem(config.serialWriteFilter,true));
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


class GwSerialLog : public GwLogWriter{
  public:
    virtual ~GwSerialLog(){}
    virtual void write(const char *data){
      usbSerial->sendToClients(data,-1); //ignore any errors
    }

};

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
  virtual void sendN2kMessage(const tN2kMsg &msg)
  {
    NMEA2000.SendMsg(msg);
  }
  virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, int sourceId)
  {
    SendNMEA0183Message(msg, sourceId);
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
};

void delayedRestart(){
  xTaskCreate([](void *p){
    delay(500);
    ESP.restart();
    vTaskDelete(NULL);
  },"reset",1000,NULL,0,NULL);
}


void startAddOnTask(TaskFunction_t task,int sourceId){
  ApiImpl* api=new ApiImpl(sourceId);
  xTaskCreate(task,"user",1000,api,3,NULL);
}

#define JSON_OK "{\"status\":\"OK\"}"

//WebServer requests that should
//be processed inside the main loop
//this prevents us from the need to sync all the accesses
class ResetRequest : public GwRequestMessage
{
public:
  ResetRequest() : GwRequestMessage(F("application/json"),F("reset")){};

protected:
  virtual void processRequest()
  {
    logger.logDebug(GwLog::LOG, "Reset Button");
    result = JSON_OK;
    delayedRestart();
  }
};

class StatusRequest : public GwRequestMessage
{
public:
  StatusRequest() : GwRequestMessage(F("application/json"),F("status")){};

protected:
  virtual void processRequest()
  {
    int numPgns = nmea0183Converter->numPgns();
    DynamicJsonDocument status(256 + numPgns * 50);
    status["numcan"] = numCan;
    status["version"] = VERSION;
    status["wifiConnected"] = gwWifi.clientConnected();
    status["clientIP"] = WiFi.localIP().toString();
    status["numClients"] = socketServer.numClients();
    status["apIp"] = gwWifi.apIP();
    nmea0183Converter->toJson(status);
    serializeJson(status, result);
  }
};

class CapabilitiesRequest : public GwRequestMessage{
  public:
    CapabilitiesRequest() : GwRequestMessage(F("application/json"),F("capabilities")){};
  protected:
    virtual void processRequest(){
      DynamicJsonDocument json(JSON_OBJECT_SIZE(6));
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
      DynamicJsonDocument json(512);
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
    for (StringMap::iterator it = args.begin(); it != args.end(); it++)
    {
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
      DynamicJsonDocument rt(100);
      rt["status"] = error;
      serializeJson(rt, result);
    }
  }
};
class ResetConfigRequest : public GwRequestMessage
{
public:
  ResetConfigRequest() : GwRequestMessage(F("application/json"),F("resetConfig")){};

protected:
  virtual void processRequest()
  {
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

void setup() {

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
    GwSerialLog *writer=new GwSerialLog();
    logger.prefix="GWSERIAL:";
    logger.setWriter(writer);
    logger.logDebug(GwLog::LOG,"created GwSerial for USB port");
  }  
  logger.logDebug(GwLog::LOG,"config: %s", config.toString().c_str());
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
  usbSerial->flush();

  logger.logDebug(GwLog::LOG,"usbRead: %s", usbReadFilter.toString().c_str());
  usbSerial->flush();

  webserver.registerMainHandler("/api/reset", [](AsyncWebServerRequest *request)->GwRequestMessage *{
    return new ResetRequest();
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
                              { return new ResetConfigRequest(); });
  webserver.registerMainHandler("/api/boatData", [](AsyncWebServerRequest *request)->GwRequestMessage *
                              { return new BoatDataRequest(); });

  webserver.begin();
  
  nmea0183Converter= N2kDataToNMEA0183::create(&logger, &boatData,&NMEA2000, 
    SendNMEA0183Message, N2K_CHANNEL_ID,config.getString(config.talkerId,String("GP")));

  toN2KConverter= NMEA0183DataToN2K::create(&logger,&boatData,[](const tN2kMsg &msg)->bool{
    logger.logDebug(GwLog::DEBUG+2,"send N2K %ld",msg.PGN);
    NMEA2000.SendMsg(msg);
    return true;
  });  
  
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  esp_efuse_read_mac(chipid);
  for (int i = 0; i < 6; i++) id += (chipid[i] << (7 * i));

  // Set product information
  NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 systemName->asCString(),  // Manufacturer's Model ID
                                 VERSION,  // Manufacturer's Software version code
                                 VERSION // Manufacturer's Model version
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
  usbSerial->flush();
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, NodeAddress);
  NMEA2000.SetForwardOwnMessages(false);
  // Set the information for other bus devices, which messages we support
  unsigned long *pgns=toN2KConverter->handledPgns();
  if (logger.isActive(GwLog::DEBUG)){
    unsigned long *op=pgns;
    while (*op != 0){
      logger.logDebug(GwLog::DEBUG,"add transmit pgn %ld",(long)(*op));
      usbSerial->flush();
      op++;
    }
  }
  NMEA2000.ExtendTransmitMessages(pgns);
  NMEA2000.ExtendReceiveMessages(nmea0183Converter->handledPgns());
  NMEA2000.SetMsgHandler([](const tN2kMsg &n2kMsg){
    numCan++;
    if ( sendSeasmart->asBoolean() ) {
      char buf[MAX_NMEA2000_MESSAGE_SEASMART_SIZE];
      if ( N2kToSeasmart(n2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE) == 0 ) return;
      socketServer.sendToClients(buf,N2K_CHANNEL_ID);
    }
    nmea0183Converter->HandleMsg(n2kMsg);
  }); 
  NMEA2000.Open();
  startAddOnTask(handleButtons,100);
  setLedMode(LED_GREEN);
  startAddOnTask(handleLeds,101);

}  
//*****************************************************************************







void sendBufferToChannels(const char * buffer, int sourceId){
  if (sendTCP->asBoolean() && checkFilter(buffer,MIN_TCP_CHANNEL_ID,false)){
    socketServer.sendToClients(buffer,sourceId);
  }
  if (sendUsb->asBoolean() && checkFilter(buffer,USB_CHANNEL_ID,false)){
    usbSerial->sendToClients(buffer,sourceId);
  }
  if (serial1 && serCanWrite && checkFilter(buffer,SERIAL1_CHANNEL_ID,false)){
    serial1->sendToClients(buffer,sourceId);
  }
}

//*****************************************************************************
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg, int sourceId) {
  if ( ! sendTCP->asBoolean() && ! sendUsb->asBoolean() ) return;
  logger.logDebug(GwLog::DEBUG+2,"SendNMEA0183(1)");
  char buf[MAX_NMEA0183_MESSAGE_SIZE+3];
  if ( !NMEA0183Msg.GetMessage(buf, MAX_NMEA0183_MESSAGE_SIZE) ) return;
  logger.logDebug(GwLog::DEBUG+2,"SendNMEA0183: %s",buf);
  size_t len=strlen(buf);
  buf[len]=0x0d;
  buf[len+1]=0x0a;
  buf[len+2]=0;
  sendBufferToChannels(buf,sourceId);
}

void handleReceivedNmeaMessage(const char *buf, int sourceId){
  if (! checkFilter(buf,sourceId,true)) return;
  if ((sourceId == USB_CHANNEL_ID && n2kFromUSB->asBoolean())||
      (sourceId >= MIN_TCP_CHANNEL_ID && n2kFromTCP->asBoolean())||
      (sourceId == SERIAL1_CHANNEL_ID && n2kFromSerial->asBoolean())
    )
    toN2KConverter->parseAndSend(buf,sourceId);
  sendBufferToChannels(buf,sourceId);
}

void handleSendAndRead(bool handleRead){
  socketServer.loop(handleRead);  
  usbSerial->loop(handleRead);
  if (serial1) serial1->loop(handleRead);
}
class NMEAMessageReceiver : public GwBufferWriter{
  uint8_t buffer[GwBuffer::RX_BUFFER_SIZE+4];
  uint8_t *writePointer=buffer;
  public:
    virtual int write(const uint8_t *buffer,size_t len){
      size_t toWrite=GwBuffer::RX_BUFFER_SIZE-(writePointer-buffer);
      if (toWrite > len) toWrite=len;
      memcpy(writePointer,buffer,toWrite);
      writePointer+=toWrite;
      *writePointer=0;
      return toWrite;
    }
    virtual void done(){
      if (writePointer == buffer) return;
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
    }
};
NMEAMessageReceiver receiver;
unsigned long lastHeapReport=0;
void loop() {
  gwWifi.loop();
  unsigned long now=millis();
  if (HEAP_REPORT_TIME > 0 && now > (lastHeapReport+HEAP_REPORT_TIME)){
    lastHeapReport=now;
    if (logger.isActive(GwLog::DEBUG)){
      logger.logDebug(GwLog::DEBUG,"Heap free=%ld, minFree=%ld",
          (long)xPortGetFreeHeapSize(),
          (long)xPortGetMinimumEverFreeHeapSize()
      );
    }
  }
  handleSendAndRead(true);
  NMEA2000.ParseMessages();

  int SourceAddress = NMEA2000.GetN2kSource();
  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    NodeAddress = SourceAddress;      // Set new Node Address (to save only once)
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    logger.logDebug(GwLog::LOG,"Address Change: New Address=%d\n", SourceAddress);
  }
  nmea0183Converter->loop();

  //read channels
  socketServer.readMessages(&receiver);
  receiver.id=USB_CHANNEL_ID;
  usbSerial->readMessages(&receiver);
  receiver.id=SERIAL1_CHANNEL_ID;
  if (serial1 && serCanRead ) serial1->readMessages(&receiver);

  //handle message requests
  GwMessage *msg=mainQueue.fetchMessage(0);
  if (msg){
    msg->process();
    msg->unref();
  }
}
