
//we only compile for some boards
#ifdef BOARD_NODEMCU32S_OBP60
#include "GwOBP60Task.h"
#include "GwApi.h"
#include "OBP60Hardware.h"              // PIN definitions
#include <Ticker.h>                     // Timer Lib for timer interrupts
#include <Wire.h>                       // I2C connections
#include <MCP23017.h>                   // MCP23017 extension Port
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <GxEPD.h>                      // GxEPD lib for E-Ink displays
#include <GxGDEW042T2/GxGDEW042T2.h>    // 4.2" Waveshare S/W 300 x 400 pixel
#include <GxIO/GxIO_SPI/GxIO_SPI.h>     // GxEPD lip for SPI display communikation
#include <GxIO/GxIO.h>                  // GxEPD lip for SPI
#include "OBP60ExtensionPort.h"         // Functions lib for extension board
#include "OBP60Keypad.h"                // Functions lib for keypad

// True type character sets
#include "Ubuntu_Bold8pt7b.h"
#include "Ubuntu_Bold20pt7b.h"
#include "Ubuntu_Bold32pt7b.h"
#include "DSEG7Classic-BoldItalic16pt7b.h"
#include "DSEG7Classic-BoldItalic42pt7b.h"
#include "DSEG7Classic-BoldItalic60pt7b.h"

// Pictures
//#include GxEPD_BitmapExamples         // Example picture
#include "MFD_OBP60_400x300_sw.h"       // MFD with logo
#include "Logo_OBP_400x300_sw.h"        // OBP Logo

#include "OBP60Data.h"                  // Data stucture
#include "OBP60QRWiFi.h"                // Functions lib for WiFi QR code
#include "Page_0.h"                     // Page 0 Depth
#include "Page_1.h"                     // Page 1 Speed
#include "Page_2.h"                     // Page 2 VBat
#include "Page_3.h"                     // Page 3 Depht / Speed
#include "OBP60Pages.h"                 // Functions lib for show pages
// new comment Adrien

tNMEA0183Msg NMEA0183Msg;
tNMEA0183 NMEA0183;

// Timer Interrupts for hardware functions
Ticker Timer1;  // Under voltage detection
Ticker Timer2;  // Keypad
Ticker Timer3;  // Binking flash LED

// Global vars
bool initComplete = false;      // Initialization complete
int taskRunCounter = 0;         // Task couter for loop section
bool gps_ready = false;         // GPS initialized and ready to use

#define INVALID_COORD -99999
class GetBoatDataRequest: public GwMessage{
    private:
        GwApi *api;
    public:
        double latitude;
        double longitude;
        GetBoatDataRequest(GwApi *api):GwMessage(F("boat data")){
            this->api=api;
        }
        virtual ~GetBoatDataRequest(){}
    protected:
        /**
         * this methos will be executed within the main thread
         * be sure not to make any time consuming or blocking operation
         */    
        virtual void processImpl(){
            //api->getLogger()->logDebug(GwLog::DEBUG,"boatData request from example task");
            /*access the values from boatData (see GwBoatData.h)
              by using getDataWithDefault it will return the given default value
              if there is no valid data available
              so be sure to use a value that never will be a valid one
              alternatively you can check using the isValid() method at each boatData item
              */
            latitude=api->getBoatData()->Latitude->getDataWithDefault(INVALID_COORD);
            longitude=api->getBoatData()->Longitude->getDataWithDefault(INVALID_COORD);
        };
};

String formatValue(GwApi::BoatValue *value){
    if (! value->valid) return "----";
    static const int bsize=30;
    char buffer[bsize+1];
    buffer[0]=0;
    if (value->getFormat() == "formatDate"){
        time_t tv=tNMEA0183Msg::daysToTime_t(value->value);
        tmElements_t parts;
        tNMEA0183Msg::breakTime(tv,parts);
        snprintf(buffer,bsize,"%02d.%02d.%04d",parts.tm_mday,parts.tm_mon+1,parts.tm_year+1900);
    }
    else if(value->getFormat() == "formatTime"){
        double inthr;
        double intmin;
        double intsec;
        double val;
        val=modf(value->value/3600.0,&inthr);
        val=modf(val*3600.0/60.0,&intmin);
        modf(val*60.0,&intsec);
//        snprintf(buffer,bsize,"%02.0f:%02.0f:%02.0f",inthr,intmin,intsec);
        snprintf(buffer,bsize,"%02.0f:%02.0f",inthr,intmin);
    }
    else if (value->getFormat() == "formatFixed0"){
        snprintf(buffer,bsize,"%.0f",value->value);
    }
    else{
        snprintf(buffer,bsize,"%.4f",value->value);
    }
    buffer[bsize]=0;
    return String(buffer);
}

// Hardware initialisation before start all services
//##################################################
void OBP60Init(GwApi *api){
    GwLog *logger=api->getLogger();

    // Define timer interrupts
//    Timer1.attach_ms(1, underVoltageDetection);     // Maximum speed with 1ms
    Timer2.attach_ms(40, readKeypad);               // Timer value nust grater than 30ms
    Timer3.attach_ms(500, blinkingFlashLED); 

    // Extension port MCP23017
    // Check I2C devices MCP23017
    Wire.begin(OBP_I2C_SDA, OBP_I2C_SCL);
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        LOG_DEBUG(GwLog::ERROR,"MCP23017 not found, check wiring");
        initComplete = false;
    }
    else{ 
        // Start communication
        mcp.init();
        mcp.portMode(MCP23017Port::A, 0b00110000);  //Port A, 0 = out, 1 = in
        mcp.portMode(MCP23017Port::B, 0b11110000);  //Port B, 0 = out, 1 = in

        // Extension Port A set defaults
        setPortPin(OBP_DIGITAL_OUT1, false);    // PA0
        setPortPin(OBP_DIGITAL_OUT2, false);    // PA1
        setPortPin(OBP_FLASH_LED, false);       // PA2
        setPortPin(OBP_BACKLIGHT_LED, false);   // PA3
        setPortPin(OBP_POWER_50, true);         // PA6
        setPortPin(OBP_POWER_33, true);         // PA7

        // Extension Port B set defaults
        setPortPin(PB0, false);     // PB0
        setPortPin(PB1, false);     // PB1
        setPortPin(PB2, false);     // PB2
        setPortPin(PB3, false);     // PB3

        // Settings for 1Wire
        bool enable1Wire = api->getConfig()->getConfigItem(api->getConfig()->use1Wire,true)->asBoolean();
        if(enable1Wire == true){
            LOG_DEBUG(GwLog::DEBUG,"1Wire Mode is On");
        }
        else{
            LOG_DEBUG(GwLog::DEBUG,"1Wire Mode is Off");
        }
        
        // Settings for backlight
        String backlightMode = api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();
        LOG_DEBUG(GwLog::DEBUG,"Backlight Mode is: %s", backlightMode);
        if(String(backlightMode) == "On"){
            setPortPin(OBP_BACKLIGHT_LED, true);
        }
        if(String(backlightMode) == "Off"){
            setPortPin(OBP_BACKLIGHT_LED, false);
        }
        if(String(backlightMode) == "Control by Key"){
            setPortPin(OBP_BACKLIGHT_LED, false);
        }

        // Settings flash LED mode
        String ledMode = api->getConfig()->getConfigItem(api->getConfig()->flashLED,true)->asString();
        LOG_DEBUG(GwLog::DEBUG,"Backlight Mode is: %s", ledMode);
        if(String(ledMode) == "Off"){
            blinkingLED = false;
        }
        if(String(ledMode) == "Limits Overrun"){
            blinkingLED = true;
        }

        // Start serial stream and take over GPS data stream form internal GPS
        bool gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asBoolean();
        if(gpsOn == true){
            Serial2.begin(9600, SERIAL_8N1, OBP_GPS_TX, -1); // GPS RX unused in hardware (-1)
            if (!Serial2) {     
                LOG_DEBUG(GwLog::ERROR,"GPS modul NEO-6M not found, check wiring");
                gps_ready = false;
                }
            else{
                LOG_DEBUG(GwLog::DEBUG,"GPS modul NEO-M6 found");
                NMEA0183.SetMessageStream(&Serial2);
                NMEA0183.Open();
                gps_ready = true;
            }
        }

        // Marker for init complete
        // Used in OBP60Task()
        initComplete = true;

        // Buzzer tone for initialization finish
        buzPower = uint(api->getConfig()->getConfigItem(api->getConfig()->buzzerPower,true)->asInt());
        buzzer(TONE4, buzPower, 500);

    }
}

// OBP60 Task
//#######################################
void OBP60Task(void *param){

    GwApi *api=(GwApi*)param;
    GwLog *logger=api->getLogger();
    GwApi::Status status;

    bool hasPosition = false;
    
    // Get configuration data from webside
    // System Settings
    api->getConfig()->getConfigItem(api->getConfig()->systemName,true)->asString().toCharArray(busInfo.systemname, 32);
    api->getConfig()->getConfigItem(api->getConfig()->systemName,true)->asString().toCharArray(busInfo.wifissid, 32);
    api->getConfig()->getConfigItem(api->getConfig()->apPassword,true)->asString().toCharArray(busInfo.wifipass, 32);
    busInfo.useadminpass = api->getConfig()->getConfigItem(api->getConfig()->useAdminPass,true)->asBoolean();
    api->getConfig()->getConfigItem(api->getConfig()->adminPassword,true)->asString().toCharArray(busInfo.adminpassword, 32);
    api->getConfig()->getConfigItem(api->getConfig()->logLevel,true)->asString().toCharArray(busInfo.loglevel, 16);
    // WiFi client settings
    busInfo.wificlienton = api->getConfig()->getConfigItem(api->getConfig()->wifiClient,true)->asBoolean();
    api->getConfig()->getConfigItem(api->getConfig()->wifiSSID,true)->asString().toCharArray(busInfo.wificlientssid, 32);
    api->getConfig()->getConfigItem(api->getConfig()->wifiPass,true)->asString().toCharArray(busInfo.wificlientpass, 32);
    // OBP60 Settings
    bool exampleSwitch = api->getConfig()->getConfigItem(api->getConfig()->obp60Config,true)->asBoolean();
    LOG_DEBUG(GwLog::DEBUG,"example switch ist %s",exampleSwitch?"true":"false");
    api->getConfig()->getConfigItem(api->getConfig()->dateFormat,true)->asString().toCharArray(busInfo.dateformat, 3);
    busInfo.timezone = api->getConfig()->getConfigItem(api->getConfig()->timeZone,true)->asInt();
    busInfo.draft = api->getConfig()->getConfigItem(api->getConfig()->draft,true)->asString().toFloat();
    busInfo.fueltank = api->getConfig()->getConfigItem(api->getConfig()->fuelTank,true)->asString().toFloat();
    busInfo.fuelconsumption = api->getConfig()->getConfigItem(api->getConfig()->fuelConsumption,true)->asString().toFloat();
    busInfo.watertank = api->getConfig()->getConfigItem(api->getConfig()->waterTank,true)->asString().toFloat();
    busInfo.wastetank = api->getConfig()->getConfigItem(api->getConfig()->wasteTank,true)->asString().toFloat();
    busInfo.batvoltage = api->getConfig()->getConfigItem(api->getConfig()->batteryVoltage,true)->asString().toFloat();
    api->getConfig()->getConfigItem(api->getConfig()->batteryType,true)->asString().toCharArray(busInfo.battype, 16);
    busInfo.batcapacity = api->getConfig()->getConfigItem(api->getConfig()->batteryCapacity,true)->asString().toFloat();
    // OBP60 Hardware
    busInfo.gps = api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asBoolean();
    busInfo.bme280 = api->getConfig()->getConfigItem(api->getConfig()->useBME280,true)->asBoolean();
    busInfo.onewire = api->getConfig()->getConfigItem(api->getConfig()->use1Wire,true)->asBoolean();
    api->getConfig()->getConfigItem(api->getConfig()->powerMode,true)->asString().toCharArray(busInfo.powermode, 16);
    busInfo.simulation = api->getConfig()->getConfigItem(api->getConfig()->useSimuData,true)->asBoolean();
    // OBP60 Display
    api->getConfig()->getConfigItem(api->getConfig()->display,true)->asString().toCharArray(busInfo.displaymode, 16);
    busInfo.statusline = api->getConfig()->getConfigItem(api->getConfig()->statusLine,true)->asBoolean();
    busInfo.refresh = api->getConfig()->getConfigItem(api->getConfig()->refresh,true)->asBoolean();
    api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString().toCharArray(busInfo.backlight, 16);
    api->getConfig()->getConfigItem(api->getConfig()->powerMode,true)->asString().toCharArray(busInfo.powermode, 16);
    // OBP60 Buzzer
    busInfo.buzerror = api->getConfig()->getConfigItem(api->getConfig()->buzzerError,true)->asBoolean();
    busInfo.buzgps = api->getConfig()->getConfigItem(api->getConfig()->buzzerGps,true)->asBoolean();
    busInfo.buzlimits = api->getConfig()->getConfigItem(api->getConfig()->buzzerLim,true)->asBoolean();
    api->getConfig()->getConfigItem(api->getConfig()->buzzerMode,true)->asString().toCharArray(busInfo.buzmode, 16);
    busInfo.buzpower = api->getConfig()->getConfigItem(api->getConfig()->buzzerPower,true)->asInt();
    // OBP60 Pages
    busInfo.numpages = api->getConfig()->getConfigItem(api->getConfig()->numberPages,true)->asInt();

    // Initializing all necessary boat data
    GwApi::BoatValue *cog=new GwApi::BoatValue(F("COG"));
    GwApi::BoatValue *twd=new GwApi::BoatValue(F("TWD"));
    GwApi::BoatValue *awd=new GwApi::BoatValue(F("AWD"));
    GwApi::BoatValue *sog=new GwApi::BoatValue(F("SOG"));
    GwApi::BoatValue *stw=new GwApi::BoatValue(F("STW"));
    GwApi::BoatValue *tws=new GwApi::BoatValue(F("TWS"));
    GwApi::BoatValue *aws=new GwApi::BoatValue(F("AWS"));
    GwApi::BoatValue *maxtws=new GwApi::BoatValue(F("MaxTws"));
    GwApi::BoatValue *maxaws=new GwApi::BoatValue(F("MaxAws"));
    GwApi::BoatValue *awa=new GwApi::BoatValue(F("AWA"));
    GwApi::BoatValue *heading=new GwApi::BoatValue(F("Heading"));
    GwApi::BoatValue *mheading=new GwApi::BoatValue(F("MagneticHeading"));
    GwApi::BoatValue *rot=new GwApi::BoatValue(F("ROT"));
    GwApi::BoatValue *variation=new GwApi::BoatValue(F("Variation"));
    GwApi::BoatValue *hdop=new GwApi::BoatValue(F("HDOP"));
    GwApi::BoatValue *pdop=new GwApi::BoatValue(F("PDOP"));
    GwApi::BoatValue *vdop=new GwApi::BoatValue(F("VDOP"));
    GwApi::BoatValue *rudderpos=new GwApi::BoatValue(F("RudderPosition"));
    GwApi::BoatValue *latitude=new GwApi::BoatValue(F("Latitude"));
    GwApi::BoatValue *longitude=new GwApi::BoatValue(F("Longitude"));
    GwApi::BoatValue *altitude=new GwApi::BoatValue(F("Altitude"));
    GwApi::BoatValue *waterdepth=new GwApi::BoatValue(F("WaterDepth"));
    GwApi::BoatValue *depthtransducer=new GwApi::BoatValue(F("DepthTransducer"));
    GwApi::BoatValue *time=new GwApi::BoatValue(F("GpsTime"));
    GwApi::BoatValue *date=new GwApi::BoatValue(F("GpsDate"));
    GwApi::BoatValue *timezone=new GwApi::BoatValue(F("Timezone"));
    GwApi::BoatValue *satinfo=new GwApi::BoatValue(F("SatInfo"));
    GwApi::BoatValue *watertemp=new GwApi::BoatValue(F("WaterTemperature"));
    GwApi::BoatValue *xte=new GwApi::BoatValue(F("XTE"));
    GwApi::BoatValue *dtw=new GwApi::BoatValue(F("DTW"));
    GwApi::BoatValue *btw=new GwApi::BoatValue(F("BTW"));
    GwApi::BoatValue *wplatitude=new GwApi::BoatValue(F("WPLatitude"));
    GwApi::BoatValue *wplongitude=new GwApi::BoatValue(F("WPLongitude"));
    GwApi::BoatValue *log=new GwApi::BoatValue(F("Log"));
    GwApi::BoatValue *triplog=new GwApi::BoatValue(F("TripLog"));

    //#################################################################

    GwApi::BoatValue *valueList[]={cog, twd, awd, sog, stw, tws, aws, maxtws, maxaws, awa, heading, mheading, rot, variation, hdop, pdop, vdop, rudderpos, latitude, longitude, altitude, waterdepth, depthtransducer, time, date, timezone, satinfo, watertemp, xte, dtw, btw, wplatitude, wplongitude, log, triplog};

    //Init E-Ink display
    display.init();                         // Initialize and clear display
    display.setTextColor(GxEPD_BLACK);      // Set display color
    display.setRotation(0);                 // Set display orientation (horizontal)
    display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
    display.update();                       // Full update (slow)
        
    if(String(busInfo.displaymode) == "Logo + QR Code" || String(busInfo.displaymode) == "Logo"){
        display.drawExampleBitmap(gImage_Logo_OBP_400x300_sw, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw start logo
//        display.drawExampleBitmap(gImage_MFD_OBP60_400x300_sw, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw start logo
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);   // Partial update (fast)
        delay(SHOW_TIME);                                // Logo show time
        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)
        if(String(busInfo.displaymode) == "Logo + QR Code"){
            qrWiFi(busInfo);                                 // Show QR code for WiFi connection
            delay(SHOW_TIME);                                // Logo show time
            display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
        }
    }

    // Task Loop
    //###############################
    while(true){
        // Task cycle time
        delay(10);  // 10ms

        // Backlight On/Off Subtask 100ms
        if((taskRunCounter % 10) == 0){
            // If key controled
            if(String(busInfo.backlight) == "Control by Key"){
                if(keystatus == "6s"){
                    LOG_DEBUG(GwLog::DEBUG,"Toggle Backlight LED");
                    togglePortPin(OBP_BACKLIGHT_LED);
                    keystatus = "0";
                }
            }
            // Change page number
            if(keystatus == "5s"){
                pageNumber ++;
                if(pageNumber > MAX_PAGE_NUMBER - 1){
                    pageNumber = 0;
                }
                first_view = true;
                keystatus = "0";
            }
            if(keystatus == "1s"){
                pageNumber --;
                if(pageNumber < 0){
                    pageNumber = MAX_PAGE_NUMBER - 1;
                }
                first_view = true;
                keystatus = "0";
            }
        }

        // Subtask all 3000ms
        if((taskRunCounter % 300) == 0){
 //           LOG_DEBUG(GwLog::DEBUG,"Subtask 2");
            //Clear swipe code
            if(keystatus == "left" || keystatus == "right"){
                keystatus = "0";
            }
        }

        // Send NMEA0183 GPS data on several bus systems
        if(busInfo.gps == true){   // If config enabled
                if(gps_ready = true){
                    tNMEA0183Msg NMEA0183Msg;
                    while(NMEA0183.GetMessage(NMEA0183Msg)){
                        api->sendNMEA0183Message(NMEA0183Msg);
                    }
                }
        }

        //######################################################################
        
        // Read the status values from gateway
        api->getStatus(status);
        busInfo.wifiApOn = status.wifiApOn;
        busInfo.wifiClientConnected = status.wifiClientConnected;
        busInfo.usbRx = status.usbRx;
        busInfo.usbTx = status.usbTx;
        busInfo.serRx = status.serRx;
        busInfo.serTx = status.serTx;
        busInfo.tcpSerRx = status.tcpSerRx;
        busInfo.tcpSerTx = status.tcpSerTx;
        busInfo.tcpClients = status.tcpClients;
        busInfo.tcpClRx = status.tcpClRx;
        busInfo.tcpClTx = status.tcpClTx;
        busInfo.tcpClientConnected = status.tcpClientConnected;
        busInfo.n2kRx = status.n2kRx;
        busInfo.n2kTx = status.n2kTx;

        //######################################################################

        // Read the current bus data and copy to stucture
        api->getBoatDataValues(35, valueList);

        busInfo.COG.fvalue = cog->value;
        cog->getFormat().toCharArray(busInfo.COG.unit, 8, 0);
        busInfo.COG.valid = int(cog->valid);

        busInfo.TWD.fvalue = twd->value;
        twd->getFormat().toCharArray(busInfo.TWD.unit, 8, 0);
        busInfo.TWD.valid = int(twd->valid);

        busInfo.AWD.fvalue = awd->value;
        awd->getFormat().toCharArray(busInfo.AWD.unit, 8, 0);
        busInfo.AWD.valid = int(awd->valid);

        busInfo.SOG.fvalue = sog->value;
        sog->getFormat().toCharArray(busInfo.SOG.unit, 8, 0);
        busInfo.SOG.valid = int(sog->valid);

        busInfo.STW.fvalue = stw->value;
        stw->getFormat().toCharArray(busInfo.STW.unit, 8, 0);
        busInfo.STW.valid = int(stw->valid);

        busInfo.TWS.fvalue = tws->value;
        tws->getFormat().toCharArray(busInfo.TWS.unit, 8, 0);
        busInfo.TWS.valid = int(tws->valid);

        busInfo.AWS.fvalue = aws->value;
        aws->getFormat().toCharArray(busInfo.AWS.unit, 8, 0);
        busInfo.AWS.valid = int(aws->valid);

        busInfo.MaxTws.fvalue = maxtws->value;
        maxtws->getFormat().toCharArray(busInfo.MaxTws.unit, 8, 0);
        busInfo.MaxTws.valid = int(maxtws->valid);

        busInfo.MaxAws.fvalue = maxaws->value;
        maxaws->getFormat().toCharArray(busInfo.MaxAws.unit, 8, 0);
        busInfo.MaxAws.valid = int(maxaws->valid);

        busInfo.AWA.fvalue = awa->value;
        awa->getFormat().toCharArray(busInfo.AWA.unit, 8, 0);
        busInfo.AWA.valid = int(awa->valid);

        busInfo.Heading.fvalue = heading->value;
        heading->getFormat().toCharArray(busInfo.Heading.unit, 8, 0);
        busInfo.Heading.valid = int(heading->valid);

        busInfo.MagneticHeading.fvalue = mheading->value;
        mheading->getFormat().toCharArray(busInfo.MagneticHeading.unit, 8, 0);
        busInfo.MagneticHeading.valid = int(mheading->valid);

        busInfo.ROT.fvalue = rot->value;
        rot->getFormat().toCharArray(busInfo.ROT.unit, 8, 0);
        busInfo.ROT.valid = int(rot->valid);

        busInfo.Variation.fvalue = variation->value;
        variation->getFormat().toCharArray(busInfo.Variation.unit, 8, 0);
        busInfo.Variation.valid = int(variation->valid);

        busInfo.HDOP.fvalue = hdop->value;
        busInfo.HDOP.valid = hdop->valid;

        busInfo.PDOP.fvalue = pdop->value;
        busInfo.PDOP.valid = pdop->valid;

        busInfo.VDOP.fvalue = vdop->value;
        busInfo.VDOP.valid = vdop->valid;

        busInfo.RudderPosition.fvalue = rudderpos->value;
        rudderpos->getFormat().toCharArray(busInfo.RudderPosition.unit, 8, 0);
        busInfo.RudderPosition.valid = int(rudderpos->valid);

        formatValue(latitude).toCharArray(busInfo.Latitude.svalue, 16, 0);
        busInfo.Latitude.valid = latitude->valid;

        formatValue(longitude).toCharArray(busInfo.Longitude.svalue, 16, 0);
        busInfo.Longitude.valid = longitude->valid;

        busInfo.Altitude.fvalue = altitude->value;
        altitude->getFormat().toCharArray(busInfo.Altitude.unit, 8, 0);
        busInfo.Altitude.valid = int(altitude->valid);

        busInfo.WaterDepth.fvalue = waterdepth->value;
        waterdepth->getFormat().toCharArray(busInfo.WaterDepth.unit, 8, 0);
        busInfo.WaterDepth.valid = int(waterdepth->valid);

        busInfo.DepthTransducer.fvalue = depthtransducer->value;
        depthtransducer->getFormat().toCharArray(busInfo.DepthTransducer.unit, 8, 0);
        busInfo.DepthTransducer.valid = int(depthtransducer->valid);

        formatValue(time).toCharArray(busInfo.Time.svalue, 16, 0);
        busInfo.Time.valid = time->valid;

        formatValue(date).toCharArray(busInfo.Date.svalue, 16, 0);
        busInfo.Date.valid = date->valid;

        busInfo.Timezone.fvalue = timezone->value;
        busInfo.Timezone.valid = timezone->valid;

        busInfo.SatInfo.fvalue = satinfo->value;
        busInfo.SatInfo.valid = satinfo->valid;

        busInfo.WaterTemperature.fvalue = watertemp->value;
        watertemp->getFormat().toCharArray(busInfo.WaterTemperature.unit, 8, 0);
        busInfo.WaterTemperature.valid = int(watertemp->valid);

        busInfo.XTE.fvalue = xte->value;
        xte->getFormat().toCharArray(busInfo.XTE.unit, 8, 0);
        busInfo.XTE.valid = int(xte->valid);

        busInfo.DTW.fvalue = dtw->value;
        dtw->getFormat().toCharArray(busInfo.DTW.unit, 8, 0);
        busInfo.DTW.valid = int(dtw->valid);

        busInfo.BTW.fvalue = btw->value;
        btw->getFormat().toCharArray(busInfo.BTW.unit, 8, 0);
        busInfo.BTW.valid = int(btw->valid);

        formatValue(wplatitude).toCharArray(busInfo.WPLatitude.svalue, 16, 0);
        busInfo.WPLatitude.valid = wplatitude->valid;

        formatValue(wplongitude).toCharArray(busInfo.WPLongitude.svalue, 16, 0);
        busInfo.WPLongitude.valid = wplongitude->valid;

        busInfo.Log.fvalue = log->value;
        log->getFormat().toCharArray(busInfo.Log.unit, 8, 0);
        busInfo.Log.valid = int(log->valid);

        busInfo.TripLog.fvalue = triplog->value;
        triplog->getFormat().toCharArray(busInfo.TripLog.unit, 8, 0);
        busInfo.TripLog.valid = int(triplog->valid);

        //######################################################################


        // Subtask all 500ms show pages
        if((taskRunCounter % 50) == 0  || first_view == true){
            LOG_DEBUG(GwLog::DEBUG,"Keystatus: %s", keystatus);
            LOG_DEBUG(GwLog::DEBUG,"Pagenumber: %d", pageNumber);
            if(String(busInfo.displaymode) == "Logo + QR Code" || String(busInfo.displaymode) == "Logo" || String(busInfo.displaymode) == "White Screen"){
                showPage(busInfo);
            }
        }

        // Subtask E-Ink full refresh
        if((taskRunCounter % (FULL_REFRESH_TIME * 100)) == 0){
            LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh");
            display.update(); 
        }

        taskRunCounter++;
    }
    vTaskDelete(NULL);
    
}

#endif
