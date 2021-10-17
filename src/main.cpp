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

#define VERSION "0.0.3"

//board specific pins
#ifdef BOARD_M5ATOM
#define ESP32_CAN_TX_PIN GPIO_NUM_22
#define ESP32_CAN_RX_PIN GPIO_NUM_19
#else
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 (Caution!!! Pin 2 before)
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4
#endif

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
#include "List.h"
#include "BoatData.h"

#include "GwLog.h"
#include "GWConfig.h"
#include "GWWifi.h"




#define ENABLE_DEBUG_LOG 0 // Debug log, set to 1 to enable AIS forward on USB-Serial / 2 for ADC voltage to support calibration
#define UDP_Forwarding 0   // Set to 1 for forwarding AIS from serial2 to UDP brodcast
#define NMEA_TO_SERIAL 1
#define HighTempAlarm 12   // Alarm level for fridge temperature (higher)
#define LowVoltageAlarm 11 // Alarm level for battery voltage (lower)


GwLog logger(LOG_SERIAL);
GwConfigHandler config(&logger);
GwWifi gwWifi(&config,&logger);


//counter
int numCan=0;

const uint16_t ServerPort = 2222; // Define the port, where server sends data. Use this e.g. on OpenCPN. Use 39150 for Navionis AIS

// UPD broadcast for Navionics, OpenCPN, etc.
const char * udpAddress = "192.168.15.255"; // UDP broadcast address. Should be the network of the ESP32 AP (please check!)
const int udpPort = 2000; // port 2000 lets think Navionics it is an DY WLN10 device

// Create UDP instance
WiFiUDP udp;

// Struct to update BoatData. See BoatData.h for content
tBoatData BoatData;

int NodeAddress;  // To store last Node Address

Preferences preferences;             // Nonvolatile storage on ESP32 - To store LastDeviceAddress

int alarmstate = false; // Alarm state (low voltage/temperature)
int acknowledge = false; // Acknowledge for alarm, button pressed


const size_t MaxClients = 10;
bool SendNMEA0183Conversion = true; // Do we send NMEA2000 -> NMEA0183 conversion
bool SendSeaSmart = false; // Do we send NMEA2000 messages in SeaSmart format

WiFiServer server(ServerPort, MaxClients);
WiFiServer json(90);

using tWiFiClientPtr = std::shared_ptr<WiFiClient>;
LinkedList<tWiFiClientPtr> clients;

tN2kDataToNMEA0183 nmea0183Converter(&NMEA2000, 0);

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = {127489L, // Engine dynamic
                                                  0
                                                 };
const unsigned long ReceiveMessages[] PROGMEM = {/*126992L,*/ // System time
      127250L, // Heading
      127258L, // Magnetic variation
      128259UL,// Boat speed
      128267UL,// Depth
      129025UL,// Position
      129026L, // COG and SOG
      129029L, // GNSS
      130306L, // Wind
      128275UL,// Log
      127245UL,// Rudder
      0
    };

// Forward declarations
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg);


WebServer webserver(80);

#define MiscSendOffset 120
#define SlowDataUpdatePeriod 1000  // Time between CAN Messages sent



// Serial port 2 config (GPIO 16)
const int baudrate = 38400;
const int rs_config = SERIAL_8N1;

// Buffer config

#define MAX_NMEA0183_MESSAGE_SIZE 150 // For AIS
char buff[MAX_NMEA0183_MESSAGE_SIZE];

// NMEA message for AIS receiving and multiplexing
tNMEA0183Msg NMEA0183Msg;
tNMEA0183 NMEA0183;


void debug_log(char* str) {
#if ENABLE_DEBUG_LOG == 1
  Serial.println(str);
#endif
}

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
  StaticJsonDocument<256> status;
  status["numcan"]=numCan;
  status["version"]=VERSION;
  status["wifiConnected"]=gwWifi.clientConnected();
  status["clientIP"]=WiFi.localIP().toString();
  String buf;
  serializeJson(status,buf);
  webserver.send(200,F("application/json"),buf);
}

void js_config(){
  webserver.send(200,F("application/json"),config.toJson());
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

void setup() {

  uint8_t chipid[6];
  uint32_t id = 0;
  int i = 0;



  // Init USB serial port
  Serial.begin(115200);
  Serial.println("Starting...");
  config.loadConfig();
  Serial.println(config.toString());
  sendUsb=config.getConfigItem(config.sendUsb,true);
  
  gwWifi.setup();

  // Start TCP server
  server.begin();

  // Start JSON server
  json.begin();

  
  // Start Web Server
  webserver.on("/", web_index);
  webserver.on("/api/reset", js_reset);
  webserver.on("/api/status", js_status);
  webserver.on("/api/config",js_config);
  webserver.on("/api/setConfig",web_setConfig);
  webserver.on("/api/resetConfig",web_resetConfig);
  webserver.onNotFound(handleNotFound);

  webserver.begin();
  Serial.println("HTTP server started");

  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega

  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  esp_efuse_read_mac(chipid);
  for (i = 0; i < 6; i++) id += (chipid[i] << (7 * i));

  // Set product information
  NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 "NMEA 2000 WiFi Gateway",  // Manufacturer's Model ID
                                 "1.0.2.25 (2019-07-07)",  // Manufacturer's Software version code
                                 "1.0.2.0 (2019-07-07)" // Manufacturer's Model version
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
  NMEA2000.ExtendReceiveMessages(ReceiveMessages);
  NMEA2000.AttachMsgHandler(&nmea0183Converter); // NMEA 2000 -> NMEA 0183 conversion
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg); // Also send all NMEA2000 messages in SeaSmart format

  nmea0183Converter.SetSendNMEA0183MessageCallback(SendNMEA0183Message);

  NMEA2000.Open();

}  
//*****************************************************************************



//*****************************************************************************
void SendBufToClients(const char *buf) {
  for (auto it = clients.begin() ; it != clients.end(); it++) {
    if ( (*it) != NULL && (*it)->connected() ) {
      (*it)->println(buf);
    }
  }
}

#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500
//*****************************************************************************
//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {

  numCan++;
  if ( !SendSeaSmart ) return;

  char buf[MAX_NMEA2000_MESSAGE_SEASMART_SIZE];
  if ( N2kToSeasmart(N2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE) == 0 ) return;
  SendBufToClients(buf);
}


//*****************************************************************************
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg) {
  if ( !SendNMEA0183Conversion ) return;

  char buf[MAX_NMEA0183_MESSAGE_SIZE];
  if ( !NMEA0183Msg.GetMessage(buf, MAX_NMEA0183_MESSAGE_SIZE) ) return;
  SendBufToClients(buf);
  if (sendUsb->asBoolean()){
    Serial.println(buf);
  }
}


bool IsTimeToUpdate(unsigned long NextUpdate) {
  return (NextUpdate < millis());
}
unsigned long InitNextUpdate(unsigned long Period, unsigned long Offset = 0) {
  return millis() + Period + Offset;
}

void SetNextUpdate(unsigned long &NextUpdate, unsigned long Period) {
  while ( NextUpdate < millis() ) NextUpdate += Period;
}


void SendN2kEngine() {
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, MiscSendOffset);
  tN2kMsg N2kMsg;

  if ( IsTimeToUpdate(SlowDataUpdated) ) {
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    SetN2kEngineDynamicParam(N2kMsg, 0, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kInt8NA, N2kInt8NA, true);
    NMEA2000.SendMsg(N2kMsg);
  }
}


//*****************************************************************************
void AddClient(WiFiClient &client) {
  Serial.println("New Client.");
  clients.push_back(tWiFiClientPtr(new WiFiClient(client)));
}

//*****************************************************************************
void StopClient(LinkedList<tWiFiClientPtr>::iterator &it) {
  Serial.println("Client Disconnected.");
  (*it)->stop();
  it = clients.erase(it);
}

//*****************************************************************************
void CheckConnections() {
  WiFiClient client = server.available();   // listen for incoming clients

  if ( client ) AddClient(client);

  for (auto it = clients.begin(); it != clients.end(); it++) {
    if ( (*it) != NULL ) {
      if ( !(*it)->connected() ) {
        StopClient(it);
      } else {
        if ( (*it)->available() ) {
          char c = (*it)->read();
          if ( c == 0x03 ) StopClient(it); // Close connection by ctrl-c
        }
      }
    } else {
      it = clients.erase(it); // Should have been erased by StopClient
    }
  }
}




void handle_json() {

  WiFiClient client = json.available();

  // Do we have a client?
  if (!client) return;

  // Serial.println(F("New client"));

  // Read the request (we ignore the content in this example)
  while (client.available()) client.read();

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<800> root;

  root["Latitude"] = BoatData.Latitude;
  root["Longitude"] = BoatData.Longitude;
  root["Heading"] = BoatData.Heading;
  root["COG"] = BoatData.COG;
  root["SOG"] = BoatData.SOG;
  root["STW"] = BoatData.STW;
  root["AWS"] = BoatData.AWS;
  root["TWS"] = BoatData.TWS;
  root["MaxAws"] = BoatData.MaxAws;
  root["MaxTws"] = BoatData.MaxTws;
  root["AWA"] = BoatData.AWA;
  root["TWA"] = BoatData.TWA;
  root["TWD"] = BoatData.TWD;
  root["TripLog"] = BoatData.TripLog;
  root["Log"] = BoatData.Log;
  root["RudderPosition"] = BoatData.RudderPosition;
  root["WaterTemperature"] = BoatData.WaterTemperature;
  root["WaterDepth"] = BoatData.WaterDepth;
  root["Variation"] = BoatData.Variation;
  root["Altitude"] = BoatData.Altitude;
  root["GPSTime"] = BoatData.GPSTime;
  root["DaysSince1970"] = BoatData.DaysSince1970;
  

  //Serial.print(F("Sending: "));
  //serializeJson(root, Serial);
  //Serial.println();

  // Write response headers
  client.println("HTTP/1.0 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  // Write JSON document
  serializeJsonPretty(root, client);

  // Disconnect
  client.stop();
}


long lastLog=millis();
void loop() {
  unsigned int size;
  int wifi_retry;
  webserver.handleClient();
  gwWifi.loop();
  handle_json();

  if (NMEA0183.GetMessage(NMEA0183Msg)) {  // Get AIS NMEA sentences from serial2

    SendNMEA0183Message(NMEA0183Msg);      // Send to TCP clients

    NMEA0183Msg.GetMessage(buff, MAX_NMEA0183_MESSAGE_SIZE); // send to buffer

#if ENABLE_DEBUG_LOG == 1
    Serial.println(buff);
#endif

#if UDP_Forwarding == 1
    size = strlen(buff);
    udp.beginPacket(udpAddress, udpPort);  // Send to UDP
    udp.write((byte*)buff, size);
    udp.endPacket();
#endif
  }

  SendN2kEngine();
  CheckConnections();
  NMEA2000.ParseMessages();

  int SourceAddress = NMEA2000.GetN2kSource();
  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    NodeAddress = SourceAddress;      // Set new Node Address (to save only once)
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    Serial.printf("Address Change: New Address=%d\n", SourceAddress);
  }

  nmea0183Converter.Update(&BoatData);

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }

}
