#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageRudderPosition : public Page
{
bool keylock = false;               // Keylock

public:
    PageRudderPosition(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageRudderPosition");
    }

    // Key functions
    virtual int handleKey(int key){
        // Keylock function
        if(key == 11){              // Code for keylock
            keylock = !keylock;     // Toggle keylock
            return 0;               // Commit the key
        }
        return key;
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData)
    {
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;

        static String unit1old = "";
        double value1 = 0.1;
        double value1old = 0.1;

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);

        // Get boat values for rudder position
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list
        String name1 = bvalue1->getName().c_str();      // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        value1 = bvalue1->value;                        // Raw value without unit convertion
        bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, commonData).unit;        // Unit of value
        if(valid1 == true){
            value1old = value1;   	                    // Save old value
            unit1old = unit1;                           // Save old unit
        }

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageRudderPosition, %s:%f", name1, value1);

        // Draw page
        //***********************************************************

        // Set background color and text color
        int textcolor = GxEPD_BLACK;
        int pixelcolor = GxEPD_BLACK;
        int bgcolor = GxEPD_WHITE;
        if(displaycolor == "Normal"){
            textcolor = GxEPD_BLACK;
            pixelcolor = GxEPD_BLACK;
            bgcolor = GxEPD_WHITE;
        }
        else{
            textcolor = GxEPD_WHITE;
            pixelcolor = GxEPD_WHITE;
            bgcolor = GxEPD_BLACK;
        }
        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

//*******************************************************************************************
        
        // Draw RudderPosition
        int rInstrument = 110;     // Radius of RudderPosition
        float pi = 3.141592;

        getdisplay().fillCircle(200, 150, rInstrument + 10, pixelcolor);    // Outer circle
        getdisplay().fillCircle(200, 150, rInstrument + 7, bgcolor);        // Outer circle
        getdisplay().fillRect(0, 30, 400, 122, bgcolor);                      // Delete half top circle

        for(int i=90; i<=270; i=i+10)
        {
            // Scaling values
            float x = 200 + (rInstrument-30)*sin(i/180.0*pi);  //  x-coordinate dots
            float y = 150 - (rInstrument-30)*cos(i/180.0*pi);  //  y-coordinate cots 
            const char *ii = " ";
            switch (i)
            {
            case 0: ii=" "; break;      // Use a blank for a empty scale value
            case 30 : ii=" "; break;
            case 60 : ii=" "; break;
            case 90 : ii="45"; break;
            case 120 : ii="30"; break;
            case 150 : ii="15"; break;
            case 180 : ii="0"; break;
            case 210 : ii="15"; break;
            case 240 : ii="30"; break;
            case 270 : ii="45"; break;
            case 300 : ii=" "; break;
            case 330 : ii=" "; break;
            default: break;
            }

            // Print text centered on position x, y
            int16_t x1, y1;     // Return values of getTextBounds
            uint16_t w, h;      // Return values of getTextBounds
            getdisplay().getTextBounds(ii, int(x), int(y), &x1, &y1, &w, &h); // Calc width of new string
            getdisplay().setCursor(x-w/2, y+h/2);
            if(i % 30 == 0){
                getdisplay().setFont(&Ubuntu_Bold8pt7b);
                getdisplay().print(ii);
            }

            // Draw sub scale with dots
            float x1c = 200 + rInstrument*sin(i/180.0*pi);
            float y1c = 150 - rInstrument*cos(i/180.0*pi);
            getdisplay().fillCircle((int)x1c, (int)y1c, 2, pixelcolor);
            float sinx=sin(i/180.0*pi);
            float cosx=cos(i/180.0*pi); 

            // Draw sub scale with lines (two triangles)
            if(i % 30 == 0){
                float dx=2;   // Line thickness = 2*dx+1
                float xx1 = -dx;
                float xx2 = +dx;
                float yy1 =  -(rInstrument-10);
                float yy2 =  -(rInstrument+10);
                getdisplay().fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                        200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),pixelcolor);
                getdisplay().fillTriangle(200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),
                        200+(int)(cosx*xx2-sinx*yy2),150+(int)(sinx*xx2+cosx*yy2),pixelcolor);  
            }

        }

        // Print label
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold16pt7b);
        getdisplay().setCursor(80, 70);
        getdisplay().print("Rudder Position");               // Label

        // Print Unit in RudderPosition
        if(valid1 == true || simulation == true){
            if(holdvalues == false){
                getdisplay().setFont(&Ubuntu_Bold12pt7b);
                getdisplay().setCursor(175, 110);
                getdisplay().print(unit1);                   // Unit
            }
            else{
                getdisplay().setFont(&Ubuntu_Bold12pt7b);
                getdisplay().setCursor(175, 110);
                getdisplay().print(unit1old);                // Unit
            }
        }
        else{
            // Print Unit of keel position
            getdisplay().setFont(&Ubuntu_Bold8pt7b);
            getdisplay().setCursor(145, 110);
            getdisplay().print("No sensor data");            // Info missing sensor
            }

        // Calculate rudder position
        if(holdvalues == true && valid1 == false){
            value1 = 2 * pi - ((value1old * 2) + pi);
        }
        else{
            value1 = 2 * pi - ((value1 * 2) + pi);
        }

        // Draw rudder position pointer
        float startwidth = 8;       // Start width of pointer

        if(valid1 == true || holdvalues == true || simulation == true){
            float sinx=sin(value1);
            float cosx=cos(value1);
            // Normal pointer
            // Pointer as triangle with center base 2*width
            float xx1 = -startwidth;
            float xx2 = startwidth;
            float yy1 = -startwidth;
            float yy2 = -(rInstrument * 0.5); 
            getdisplay().fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),pixelcolor);   
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument * 0.5);
            float iy2 = -endwidth;
            getdisplay().fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),pixelcolor);
        }

        // Center circle
        getdisplay().fillCircle(200, 150, startwidth + 6, bgcolor);
        getdisplay().fillCircle(200, 150, startwidth + 4, pixelcolor);

//*******************************************************************************************
        // Key Layout
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            getdisplay().setCursor(130, 290);
            getdisplay().print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                getdisplay().setCursor(343, 290);
                getdisplay().print("[ILUM]");
            }
        }
        else{
            getdisplay().setCursor(130, 290);
            getdisplay().print(" [    Keylock active    ]");
        }

        // Update display
        getdisplay().nextPage();    // Partial update (fast)
    };
};

static Page *createPage(CommonData &common){
    return new PageRudderPosition(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageRudderPosition(
    "RudderPosition",   // Page name
    createPage,         // Action
    0,                  // Number of bus values depends on selection in Web configuration
    {"RPOS"},           // Bus values we need in the page
    true                // Show display header on/off
);

#endif