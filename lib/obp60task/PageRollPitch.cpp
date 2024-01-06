#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageRollPitch : public Page
{
bool keylock = false;               // Keylock

public:
    PageRollPitch(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageRollPitch");
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

        static String svalue1old = "";
        static String unit1old = "";
        static String svalue2old = "";
        static String unit2old = "";

        double value1 = 0;
        double value2 = 0;
        String svalue1 = "";
        String unit1 = "";
        String svalue2 = "";
        String unit2 = "";

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);
        int rolllimit = config->getInt(config->rollLimit);
        String roffset = config->getString(config->rollOffset);
        double rolloffset = roffset.toFloat()/360*(2*PI);
        String poffset = config->getString(config->pitchOffset);
        double pitchoffset = poffset.toFloat()/360*(2*PI);

        // Get boat values for roll
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (xdrRoll)
        String name1 = xdrDelete(bvalue1->getName());   // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        bool valid1 = bvalue1->valid;                   // Valid information
        if(valid1 == true){
            value1 = bvalue1->value + rolloffset;       // Raw value for pitch
        }
        else{
            if(simulation == true){
                value1 = (20 + float(random(0, 50)) / 10.0)/360*2*PI;
                unit1 = "Deg";
            }
            else{
                value1 = 0;
            }
        }
        if(value1/(2*PI)*360 > -10 && value1/(2*PI)*360 < 10){
            svalue1 = String(value1/(2*PI)*360,1);      // Convert raw value to string
        }
        else{
            svalue1 = String(value1/(2*PI)*360,0);
        }
        unit1 = formatValue(bvalue1, commonData).unit;  // Unit of value
        if(valid1 == true){
            svalue1old = svalue1;   	                // Save old value
            unit1old = unit1;                           // Save old unit
        }

        // Get boat values for pitch
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // Second element in list (xdrPitch)
        String name2 = xdrDelete(bvalue2->getName());   // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        bool valid2 = bvalue2->valid;                   // Valid information
        if(valid2 == true){
            value2 = bvalue2->value + pitchoffset;      // Raw value for pitch
        }
        else{
             if(simulation == true){
                value2 = (float(random(-5, 5)))/360*2*PI;
                unit2 = "Deg";
            }
            else{
                value2 = 0;
            }
        }
        if(value2/(2*PI)*360 > -10 && value2/(2*PI)*360 < 10){
            svalue2 = String(value2/(2*PI)*360,1);      // Convert raw value to string
        }
        else{
            svalue2 = String(value2/(2*PI)*360,0);
        }
        unit2 = formatValue(bvalue2, commonData).unit;  // Unit of value
        if(valid2 == true){
            svalue2old = svalue2;   	                // Save old value
            unit2old = unit2;                           // Save old unit
        }

        // Optical warning by limit violation
        if(String(flashLED) == "Limit Violation"){
            // Limits for roll
            if(value1*360/(2*PI) > -1*rolllimit && value1*360/(2*PI) < rolllimit){
                setBlinkingLED(false);
                setFlashLED(false);
            }
            else{
                setBlinkingLED(true);
            } 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageRollPitch, %s:%f,  %s:%f", name1, value1, name2, value2);

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
        // Clear display by call in obp60task.cpp in main loop

        // Horizintal separator left
        display.fillRect(0, 149, 60, 3, pixelcolor);

        // Show roll value
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(10, 270);
        if(holdvalues == false) display.print(svalue1); // Value
        else display.print(svalue1old);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(10, 220);
        display.print(name1);                           // Name
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(10, 190);
        display.print(" ");
        if(holdvalues == false) display.print(unit1);   // Unit
        else display.print(unit1old);

        // Horizintal separator right
        display.fillRect(340, 149, 80, 3, pixelcolor);

        // Show pitch value
        display.setTextColor(textcolor);
        display.setFont(&DSEG7Classic_BoldItalic20pt7b);
        display.setCursor(295, 270);
        if(holdvalues == false) display.print(svalue2); // Value
        else display.print(svalue2old);
        display.setFont(&Ubuntu_Bold12pt7b);
        display.setCursor(335, 220);
        display.print(name2);                           // Name
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(335, 190);
        display.print(" ");
        if(holdvalues == false) display.print(unit1);   // Unit
        else display.print(unit1old);

//*******************************************************************************************
        
        // Draw instrument
        int rInstrument = 100;     // Radius of instrument
        float pi = 3.141592;

        display.fillCircle(200, 150, rInstrument + 10, pixelcolor);    // Outer circle
        display.fillCircle(200, 150, rInstrument + 7, bgcolor);        // Outer circle     

        for(int i=0; i<360; i=i+10)
        {
            // Only scaling +/- 60 degrees
            if((i >= 0 && i <= 60) ||  (i >= 300 && i <= 360)){
                // Scaling values
                float x = 200 + (rInstrument+25)*sin(i/180.0*pi);  //  x-coordinate dots
                float y = 150 - (rInstrument+25)*cos(i/180.0*pi);  //  y-coordinate cots 
                const char *ii = "";
                switch (i)
                {
                case 0: ii="0"; break;
                case 20 : ii="20"; break;
                case 40 : ii="40"; break;
                case 60 : ii="60"; break;
                case 300 : ii="60"; break;
                case 320 : ii="40"; break;
                case 340 : ii="20"; break;
                default: break;
                }

                // Print text centered on position x, y
                int16_t x1, y1;     // Return values of getTextBounds
                uint16_t w, h;      // Return values of getTextBounds
                display.getTextBounds(ii, int(x), int(y), &x1, &y1, &w, &h); // Calc width of new string
                display.setCursor(x-w/2, y+h/2);
                if(i % 20 == 0){
                    display.setFont(&Ubuntu_Bold8pt7b);
                    display.print(ii);
                }

                // Draw sub scale with dots
                float x1c = 200 + rInstrument*sin(i/180.0*pi);
                float y1c = 150 - rInstrument*cos(i/180.0*pi);
                display.fillCircle((int)x1c, (int)y1c, 2, pixelcolor);
                float sinx=sin(i/180.0*pi);
                float cosx=cos(i/180.0*pi); 

                // Draw sub scale with lines (two triangles)
                if(i % 20 == 0){
                    float dx=2;   // Line thickness = 2*dx+1
                    float xx1 = -dx;
                    float xx2 = +dx;
                    float yy1 =  -(rInstrument-10);
                    float yy2 =  -(rInstrument+10);
                    display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                            200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                            200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),pixelcolor);
                    display.fillTriangle(200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                            200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),
                            200+(int)(cosx*xx2-sinx*yy2),150+(int)(sinx*xx2+cosx*yy2),pixelcolor);  
                }
            }
        }

        // Draw mast position pointer
        float startwidth = 8;           // Start width of pointer

//        value1 = (2 * pi ) - value1;    // Mirror coordiante system for pointer, keel and boat

        if(valid1 == true || holdvalues == true || simulation == true){
            float sinx=sin(value1 + pi);
            float cosx=cos(value1 + pi);
            // Normal pointer
            // Pointer as triangle with center base 2*width
            float xx1 = -startwidth;
            float xx2 = startwidth;
            float yy1 = -startwidth;
            float yy2 = -(rInstrument * 0.7); 
            display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),pixelcolor);   
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument * 0.7);
            float iy2 = -endwidth;
            display.fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),pixelcolor);

            // Draw counterweight
            display.fillCircle(200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2), 5, pixelcolor);
        }

        // Center circle
        display.fillCircle(200, 150, startwidth + 22, bgcolor);
        display.fillCircle(200, 150, startwidth + 20, pixelcolor);      // Boat circle
        int x0 = 200;
        int y0 = 150;
        int x1 = x0 + 50*cos(value1);
        int y1 = y0 + 50*sin(value1);
        int x2 = x0 + 50*cos(value1 - pi/2);
        int y2 = y0 + 50*sin(value1 - pi/2);
        display.fillTriangle(x0, y0, x1, y1, x2, y2, bgcolor);          // Clear half top side of boat circle (right triangle)
        x1 = x0 + 50*cos(value1 + pi);
        y1 = y0 + 50*sin(value1 + pi);
        display.fillTriangle(x0, y0, x1, y1, x2, y2, bgcolor);          // Clear half top side of boat circle (left triangle)
        display.fillRect(150, 160, 100, 4, pixelcolor);                 // Water line

        // Draw roll pointer
        startwidth = 4;     // Start width of pointer
        if(valid1 == true || holdvalues == true || simulation == true){
            float sinx=sin(value1);     // Roll
            float cosx=cos(value1);
            // Normal pointer
            // Pointer as triangle with center base 2*width
            float xx1 = -startwidth;
            float xx2 = startwidth;
            float yy1 = -startwidth;
            float yy2 = -(rInstrument - 15); 
            display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),pixelcolor);   
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument - 15);
            float iy2 = -endwidth;
            display.fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),pixelcolor);
        }
        else{
            // Print sensor info
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(145, 200);
            display.print("No sensor data");            // Info missing sensor
        }

//*******************************************************************************************
        // Key Layout
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            display.setCursor(130, 290);
            display.print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                display.setCursor(343, 290);
                display.print("[ILUM]");
            }
        }
        else{
            display.setCursor(130, 290);
            display.print(" [    Keylock active    ]");
        }

        // Update display
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)

    };
};

static Page *createPage(CommonData &common){
    return new PageRollPitch(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageRollPitch(
    "RollPitch",        // Page name
    createPage,         // Action
    0,                  // Number of bus values depends on selection in Web configuration
    {"xdrRoll", "xdrPitch"},// Bus values we need in the page
    true                // Show display header on/off
);

#endif