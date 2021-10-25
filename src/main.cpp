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

#define VERSION "0.1.0"
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

#include "N2kDataToNMEA0183.h"


#include "GwLog.h"
#include "GWConfig.h"
#include "GWWifi.h"
#include "GwSocketServer.h"
#include "GwBoatData.h"
#include "GwMessage.h"
#include "GwSerial.h"

typedef std::map<String,String> StringMap;


GwLog logger(GwLog::DEBUG,NULL);
GwConfigHandler config(&logger);
GwWifi gwWifi(&config,&logger);
GwSocketServer socketServer(&config,&logger);
GwBoatData boatData(&logger);


//counter
int numCan=0;


int NodeAddress;  // To store last Node Address

Preferences preferences;             // Nonvolatile storage on ESP32 - To store LastDeviceAddress

bool SendNMEA0183Conversion = true; // Do we send NMEA2000 -> NMEA0183 conversion
bool SendSeaSmart = false; // Do we send NMEA2000 messages in SeaSmart format

N2kDataToNMEA0183 *nmea0183Converter=N2kDataToNMEA0183::create(&logger, &boatData,&NMEA2000, 0);

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = {127489L, // Engine dynamic
                                                  0
};
// Forward declarations
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg);

AsyncWebServer webserver(80);

// Serial port 2 config (GPIO 16)
const int baudrate = 38400;
const int rs_config = SERIAL_8N1;

// Buffer config

#define MAX_NMEA0183_MESSAGE_SIZE 150 // For AIS
char buff[MAX_NMEA0183_MESSAGE_SIZE];


tNMEA0183 NMEA0183;

QueueHandle_t queue=xQueueCreate(10,sizeof(Message *));
void handleAsyncWebRequest(AsyncWebServerRequest *request, RequestMessage *msg, String contentType)
{
  msg->ref(); //for the queue
  if (!xQueueSend(queue, &msg, 0))
  {
    Serial.println("unable to enqueue");
    msg->unref(); //queue
    msg->unref(); //our
    request->send(500, "text/plain", "queue full");
    return;
  }
  logger.logDebug(GwLog::DEBUG + 1, "wait queue");
  if (msg->wait(500))
  {
    logger.logDebug(GwLog::DEBUG + 1, "request ok");
    request->send(200, contentType, msg->getResult());
    msg->unref();
    return;
  }
  logger.logDebug(GwLog::DEBUG + 1, "switching to async");
  //msg is handed over to async handling
  bool finished = false;
  AsyncWebServerResponse *r = request->beginChunkedResponse(
      contentType, [msg, finished](uint8_t *ptr, size_t len, size_t len2) -> size_t
      {
        logger.logDebug(GwLog::DEBUG + 1, "try read");
        if (msg->isHandled() || msg->wait(1))
        {
          int rt = msg->consume(ptr, len);
          logger.logDebug(GwLog::DEBUG + 1, "async response available, return %d\n", rt);
          return rt;
        }
        else
          return RESPONSE_TRY_AGAIN;
      },
      NULL);
  request->onDisconnect([msg](void)
                        {
                          logger.logDebug(GwLog::DEBUG + 1, "onDisconnect");
                          msg->unref();
                        });
  request->send(r);
}

#define JSON_OK "{\"status\":\"OK\"}"
//embedded files
extern const uint8_t indexFile[] asm("_binary_web_index_html_gz_start"); 
extern const uint8_t indexFileEnd[] asm("_binary_web_index_html_gz_end"); 
extern const uint8_t indexFileLen[] asm("_binary_web_index_html_gz_size");


String js_status(){
  int numPgns=nmea0183Converter->numPgns();
  DynamicJsonDocument status(256+numPgns*50);
  status["numcan"]=numCan;
  status["version"]=VERSION;
  status["wifiConnected"]=gwWifi.clientConnected();
  status["clientIP"]=WiFi.localIP().toString();
  status["numClients"]=socketServer.numClients();
  status["apIp"]=gwWifi.apIP();
  nmea0183Converter->toJson(status);
  String buf;
  serializeJson(status,buf);
  return buf;
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

GwConfigInterface *sendUsb=NULL;
GwConfigInterface *sendTCP=NULL;
GwConfigInterface *sendSeasmart=NULL;
GwConfigInterface *systemName=NULL;

GwSerial usbSerial(&logger, UART_NUM_0);
class GwSerialLog : public GwLogWriter{
  public:
    virtual ~GwSerialLog(){}
    virtual void write(const char *data){
      usbSerial.enqueue((const uint8_t*)data,strlen(data)); //ignore any errors
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
  int st=usbSerial.setup(baud,3,1); //TODO: PIN defines
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
  sendUsb=config.getConfigItem(config.sendUsb,true);
  sendTCP=config.getConfigItem(config.sendTCP,true);
  sendSeasmart=config.getConfigItem(config.sendSeasmart,true);
  systemName=config.getConfigItem(config.systemName,true);
  MDNS.begin(config.getConfigItem(config.systemName)->asCString());
  gwWifi.setup();

  // Start TCP server
  socketServer.begin();

  // Start Web Server
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response=request->beginResponse_P(200,"text/html",(const uint8_t *)indexFile,(int)indexFileLen);
      response->addHeader(F("Content-Encoding"), F("gzip"));
      request->send(response);
  });
  webserver.on("/api/reset", HTTP_GET,[](AsyncWebServerRequest *request){
    logger.logDebug(GwLog::LOG,"Reset Button");
    ESP.restart();
  });
  class StatusRequest : public RequestMessage{
    public:
      StatusRequest(): RequestMessage(){};
    protected:
      virtual void processRequest(){
          result=js_status();
      }
  };
  webserver.on("/api/status",HTTP_GET,[](AsyncWebServerRequest *request){
    StatusRequest *msg=new StatusRequest();
    handleAsyncWebRequest(request,msg,F("application/json"));
  });
  class ConfigRequest : public RequestMessage{
    public:
      ConfigRequest(): RequestMessage(){};
    protected:
      virtual void processRequest(){
          result=config.toJson();
      }
  };
  webserver.on("/api/config",HTTP_GET,[](AsyncWebServerRequest *request){
    RequestMessage *msg=new ConfigRequest();
    handleAsyncWebRequest(request,msg,F("application/json")); 
  });

  class SetConfigRequest : public RequestMessage{
    public:
      SetConfigRequest(): RequestMessage(){};
      StringMap args;
    protected:
      virtual void processRequest(){
        bool ok=true;
        String error;
        for (StringMap::iterator it=args.begin();it != args.end();it++){
          bool rt=config.updateValue(it->first,it->second);
          if (! rt){
            logger.logString("ERR: unable to update %s to %s",it->first.c_str(),it->second.c_str());
            ok=false;
            error+=it->first;
            error+="=";
            error+=it->second;
            error+=",";
          }
        }
        if (ok){
          result=JSON_OK;
          logger.logString("update config and restart");
          config.saveConfig();
          delay(100);
          ESP.restart();
        }
        else{
          DynamicJsonDocument rt(100);
          rt["status"]=error;
          serializeJson(rt,result);
        }   
      }
  };
  webserver.on("/api/setConfig",HTTP_GET,[](AsyncWebServerRequest *request){
    StringMap args;
    for (int i=0;i<request->args();i++){
      args[request->argName(i)]=request->arg(i);
    }
    SetConfigRequest *msg=new SetConfigRequest();
    msg->args=args;
    handleAsyncWebRequest(request,msg,F("application/json")); 
  });
  class ResetConfigRequest : public RequestMessage{
    public:
      ResetConfigRequest(): RequestMessage(){};
    protected:
      virtual void processRequest(){
        config.reset(true);
        logger.logString("reset config, restart");
        result=JSON_OK;
        delay(100);
        ESP.restart();
      }
  };
  webserver.on("/api/resetConfig",HTTP_GET,[](AsyncWebServerRequest *request){
    RequestMessage *msg=new ResetConfigRequest();
    handleAsyncWebRequest(request,msg,F("application/json"));   
  });
  class BoatDataRequest : public RequestMessage{
    public:
      BoatDataRequest(): RequestMessage(){};
    protected:
      virtual void processRequest(){
          result=boatData.toJson();
      }
  };
  webserver.on("/api/boatData",HTTP_GET,[](AsyncWebServerRequest *request){
    RequestMessage *msg=new BoatDataRequest();
    handleAsyncWebRequest(request,msg,F("application/json"));   
  });
  webserver.onNotFound(notFound);
  webserver.begin();
  logger.logDebug(GwLog::LOG,"HTTP server started");

  MDNS.addService("_http","_tcp",80);

  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega

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

  logger.logDebug(GwLog::LOG,"NodeAddress=%d\n", NodeAddress);

  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, NodeAddress);

  NMEA2000.ExtendTransmitMessages(TransmitMessages);
  NMEA2000.ExtendReceiveMessages(nmea0183Converter->handledPgns());
  NMEA2000.AttachMsgHandler(nmea0183Converter); // NMEA 2000 -> NMEA 0183 conversion
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg); // Also send all NMEA2000 messages in SeaSmart format

  nmea0183Converter->SetSendNMEA0183MessageCallback(SendNMEA0183Message);

  NMEA2000.Open();

}  
//*****************************************************************************





#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500
//*****************************************************************************
//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {

  numCan++;
  if ( !sendSeasmart->asBoolean() ) return;

  char buf[MAX_NMEA2000_MESSAGE_SEASMART_SIZE];
  if ( N2kToSeasmart(N2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE) == 0 ) return;
  socketServer.sendToClients(buf);
}


//*****************************************************************************
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg) {
  if ( ! sendTCP->asBoolean() && ! sendUsb->asBoolean() ) return;

  char buf[MAX_NMEA0183_MESSAGE_SIZE];
  if ( !NMEA0183Msg.GetMessage(buf, MAX_NMEA0183_MESSAGE_SIZE) ) return;
  if (sendTCP->asBoolean()){
    socketServer.sendToClients(buf);
  }
  if (sendUsb->asBoolean()){
    int len=strlen(buf);
    if (len >= (MAX_NMEA0183_MESSAGE_SIZE -2)) return;
    buf[len]=0x0d;
    len++;
    buf[len]=0x0a;
    len++;
    buf[len]=0;
    usbSerial.enqueue((const uint8_t*)buf,len);
  }
}

void loop() {
  gwWifi.loop();

  socketServer.loop();
  NMEA2000.ParseMessages();

  int SourceAddress = NMEA2000.GetN2kSource();
  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    NodeAddress = SourceAddress;      // Set new Node Address (to save only once)
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    Serial.printf("Address Change: New Address=%d\n", SourceAddress);
  }
  nmea0183Converter->loop();

  //handle messages from the async web server
  Message *msg=NULL;
  if (xQueueReceive(queue,&msg,0)){
    logger.logDebug(GwLog::DEBUG+1,"main message");
    msg->process();
    msg->unref();
  }
  if (usbSerial.write() == GwBuffer::ERROR){
    logger.logDebug(GwLog::DEBUG,"overflow in USB serial");
  }
  usbSerial.read();

}
