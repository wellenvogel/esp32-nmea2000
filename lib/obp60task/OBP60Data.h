#ifndef _OBP60Data_H
#define _OBP60Data_H

#include <Arduino.h>

typedef struct{               // Sub structure for bus data
  float fvalue = 0;           // Float value
  char svalue[16] = "";       // Char value
  char unit[8] = "";          // Unit
  bool valid = 0;             // Valid flag
} dataContainer;

typedef struct{
  // Gateway status infos
  bool wifiApOn = false;                    // Status access point [on|off]
  bool wifiClientConnected = false;         // Client connected [yes|no]
  unsigned long usbRx = 0;                  // USB receive traffic
  unsigned long usbTx = 0;                  // USB send traffic
  unsigned long serRx = 0;                  // MNEA0183 serial receive traffic
  unsigned long serTx = 0;                  // NMEA0183 serial send traffic
  unsigned long tcpSerRx = 0;               // MNEA0183 TCP server receive traffic
  unsigned long tcpSerTx = 0;               // MNEA0183 TCP server send traffic
  int tcpClients = 0;                       // Number of connected TCP clients
  unsigned long tcpClRx = 0;                // MNEA0183 TCP client receive traffic
  unsigned long tcpClTx = 0;                // MNEA0183 TCP client send traffic
  bool tcpClientConnected = false;          // Connected TCP clients
  unsigned long n2kRx = 0;                  // NMEA2000 CAN receive traffic
  unsigned long n2kTx = 0;                  // NMEA2000 CAN send traffic
  // System Settings
  char systemname[32] = "";                 // System name show on web page and mDNS name
  char wifissid[32] = "";                   // WiFi access point SSID
  char wifipass[32] = "";                   // WiFi access point password
  bool useadminpass = false;                // Use admin password [on|off]
  char adminpassword[32] = "";              // Admin password
  char loglevel[16] = "";                   // Loglevel [off|error|log|debug]
  // WiFi client settings
  bool wificlienton = false;                // Is WiFi client on [on|off]
  char wificlientssid[32] = "";             // Wifi client SSID
  char wificlientpass[32] = "";             // Wifi client password
  // OBP60 Settings
  char dateformat[3] = "GB";                // Date format for status line [DE|GB|US]
  int timezone = 0;                         // Time zone [-12...+12]
  float draft = 0;                          // Boat draft up to keel [m]
  float fueltank = 0;                       // Fuel tank capacity [0...10m]
  float fuelconsumption = 0;                // Fuel consumption [0...1000l/min]
  float watertank = 0;                      // Water tank kapacity [0...5000l]
  float wastetank = 0;                      // Waste tank kapacity [0...5000l]
  float batvoltage = 0;                     // Battery voltage [0...1000V]
  char battype[16] = "Pb";                  // Battery type [Pb|Gel|AGM|LiFePo4]
  float batcapacity = 0;                    // Battery capacity [0...10000Ah]
  // OBP60 Hardware
  bool gps = false;                         // Internal GPS [on|off]
  bool bme280 = false;                      // Internat BME280 [on|off]
  bool onewire = false;                     // Internal 1Wire bus [on|off]
  char powermode[16] = "Max Power";         // Power mode [Max Power|Only 3.3V|Only 5.0V|Min Power]
  bool simulation = false;                  // Simulation data [on|off]
  // OBP60 Display
  char displaymode[16] = "Logo + QR Code";  // Dislpay mode [White Screen|Logo|Logo + QR Code|Off]
  bool statusline = true;                   // Show status line [on|off]
  bool refresh = false;                     // Refresh display after select a new page [on|off]
  char backlight[16] = "Control by Key";    // Backlight mode [Off|Control by Sun|Control by Bus|Control by Time|Control by Key|On]
  char flashled[16] = "Off";                // Flash LED mode [Off|Bus Data|GPX Fix|Limits Overrun]
  // OBP60 Buzzer
  bool buzerror = false;                    // Buzzer error [on|off]
  bool buzgps = false;                      // Buzzer by GPS error [on|off]
  bool buzlimits = false;                   // Buzzer by limit underruns and overruns [on|off]
  char buzmode[16] = "Off";                 // Buzzer mode [Off|Short Single Beep|Lond Single Beep|Beep until Confirmation]
  int buzpower = 0;                         // Buzzer power [0...100%]
  // OBP60 Pages
  int numpages = 1;                         // Numper of listed pages
  // Bus data
  dataContainer AWA;
  dataContainer AWD;
  dataContainer AWS;
  dataContainer Altitude;
  dataContainer BTW;
  dataContainer COG;
  dataContainer DTW;
  dataContainer Date;
  dataContainer DepthTransducer;
  dataContainer Deviation;
  dataContainer HDOP;
  dataContainer Heading;
  dataContainer Latitude;
  dataContainer Log;
  dataContainer Longitude;
  dataContainer MagneticHeading;
  dataContainer MaxAws;
  dataContainer MaxTws;
  dataContainer PDOP;
  dataContainer ROT;
  dataContainer RudderPosition;
  dataContainer SOG;
  dataContainer STW;
  dataContainer SatInfo;
  dataContainer Time;
  dataContainer TWD;
  dataContainer TWS;
  dataContainer Timezone;
  dataContainer TripLog;
  dataContainer VDOP;
  dataContainer Variation;
  dataContainer WPLatitude;
  dataContainer WPLongitude;
  dataContainer WaterDepth;
  dataContainer WaterTemperature;
  dataContainer XTE;
} busData;

busData busInfo;

#endif