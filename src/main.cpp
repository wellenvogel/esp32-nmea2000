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

#define VERSION "0.0.7"
#include "GwHardware.h"

#define LOG_SERIAL true

#include <Arduino.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <Seasmart.h>
#include <N2kMessages.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#include "N2kDataToNMEA0183.h"


#include "GwLog.h"
#include "GWConfig.h"
#include "GWWifi.h"
#include "GwSocketServer.h"
#include "GwBoatData.h"


GwLog logger(LOG_SERIAL);
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


WebServer webserver(80);

// Serial port 2 config (GPIO 16)
const int baudrate = 38400;
const int rs_config = SERIAL_8N1;

// Buffer config

#define MAX_NMEA0183_MESSAGE_SIZE 150 // For AIS
char buff[MAX_NMEA0183_MESSAGE_SIZE];


tNMEA0183 NMEA0183;



#define JSON_OK "{\"status\":\"OK\"}"
//embedded files
extern const uint8_t indexFile[] asm("_binary_web_index_html_gz_start"); 
extern const uint8_t indexFileEnd[] asm("_binary_web_index_html_gz_end"); 
extern const uint8_t indexFileLen[] asm("_binary_web_index_html_gz_size");

void web_index()    // Wenn "http://<ip address>/" aufgerufen wurde
{
  webserver.sendHeader(F("Content-Encoding"), F("gzip"));
  webserver.send_P(200, "text/html", (const char *)indexFile,(int)indexFileLen);  //dann Index Webseite senden
}

void js_reset()      // Wenn "http://<ip address>/gauge.min.js" aufgerufen wurde
{
  Serial.println("Reset Button");
  ESP.restart();
}


void js_status(){
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
  webserver.send(200,F("application/json"),buf);
}

void js_config(){
  webserver.send(200,F("application/json"),config.toJson());
}

void js_boatData(){
  webserver.send(200,F("application/json"),boatData.toJson());
}

void web_setConfig(){
  bool ok=true;
  String error;
  for (int i=0;i<webserver.args();i++){
    String v=webserver.arg(i);
    String n=webserver.argName(i);
    bool rt=config.updateValue(n,v);
    if (! rt){
      logger.logString("ERR: unable to update %s to %s",n.c_str(),v.c_str());
      ok=false;
      error+=n;
      error+="=";
      error+=v;
      error+=",";
    }
  }
  if (ok){
    webserver.send(200,F("application/json"),JSON_OK);
    logger.logString("update config and restart");
    config.saveConfig();
    delay(100);
    ESP.restart();
  }
  else{
    DynamicJsonDocument rt(100);
    rt["status"]=error;
    String buf;
    serializeJson(rt,buf);
    webserver.send(200,F("application/json"),buf);
  }
}
void web_resetConfig(){
  config.reset(true);
  logger.logString("reset config, restart");
  webserver.send(200,F("application/json"),JSON_OK);
  delay(100);
  ESP.restart();
}

void handleNotFound()
{
  webserver.send(404, F("text/plain"), "File Not Found\n\n");
}


GwConfigInterface *sendUsb=NULL;
GwConfigInterface *sendTCP=NULL;
GwConfigInterface *sendSeasmart=NULL;
GwConfigInterface *systemName=NULL;

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
  Serial.begin(baud);
  Serial.println("Starting...");
  Serial.println(config.toString());
  sendUsb=config.getConfigItem(config.sendUsb,true);
  sendTCP=config.getConfigItem(config.sendTCP,true);
  sendSeasmart=config.getConfigItem(config.sendSeasmart,true);
  systemName=config.getConfigItem(config.systemName,true);
  
  gwWifi.setup();

  // Start TCP server
  socketServer.begin();

  // Start Web Server
  webserver.on("/", web_index);
  webserver.on("/api/reset", js_reset);
  webserver.on("/api/status", js_status);
  webserver.on("/api/config",js_config);
  webserver.on("/api/setConfig",web_setConfig);
  webserver.on("/api/resetConfig",web_resetConfig);
  webserver.on("/api/boatData",js_boatData);
  webserver.onNotFound(handleNotFound);
  webserver.begin();
  Serial.println("HTTP server started");

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

  Serial.printf("NodeAddress=%d\n", NodeAddress);

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
    Serial.println(buf);
  }
}

void loop() {
  webserver.handleClient();
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

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }

}
