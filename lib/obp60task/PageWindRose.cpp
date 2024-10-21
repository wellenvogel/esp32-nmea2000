#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageWindRose : public Page
{
bool keylock = false;               // Keylock
int16_t lp = 80;                    // Pointer length

public:
    PageWindRose(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageWindRose");
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
        static String svalue3old = "";
        static String unit3old = "";
        static String svalue4old = "";
        static String unit4old = "";
        static String svalue5old = "";
        static String unit5old = "";
        static String svalue6old = "";
        static String unit6old = "";

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);

        // Get boat values for AWA
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = xdrDelete(bvalue1->getName());   // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        double value1 = bvalue1->value;                 // Value as double in SI unit
        bool valid1 = bvalue1->valid;                   // Valid information
        value1 = formatValue(bvalue1, commonData).value;// Format only nesaccery for simulation data for pointer
        String svalue1 = formatValue(bvalue1, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, commonData).unit;        // Unit of value
        if(valid1 == true){
            svalue1old = svalue1;   	                // Save old value
            unit1old = unit1;                           // Save old unit
        }

        // Get boat values for AWS
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // First element in list (only one value by PageOneValue)
        String name2 = xdrDelete(bvalue2->getName());   // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        double value2 = bvalue2->value;                 // Value as double in SI unit
        bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, commonData).unit;        // Unit of value
        if(valid2 == true){
            svalue2old = svalue2;   	                // Save old value
            unit2old = unit2;                           // Save old unit
        }

        // Get boat values TWD
        GwApi::BoatValue *bvalue3 = pageData.values[2]; // Second element in list (only one value by PageOneValue)
        String name3 = xdrDelete(bvalue3->getName());   // Value name
        name3 = name3.substring(0, 6);                  // String length limit for value name
        double value3 = bvalue3->value;                 // Value as double in SI unit
        bool valid3 = bvalue3->valid;                   // Valid information 
        String svalue3 = formatValue(bvalue3, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit3 = formatValue(bvalue3, commonData).unit;        // Unit of value
        if(valid3 == true){
            svalue3old = svalue3;   	                // Save old value
            unit3old = unit3;                           // Save old unit
        }

        // Get boat values TWS
        GwApi::BoatValue *bvalue4 = pageData.values[3]; // Second element in list (only one value by PageOneValue)
        String name4 = xdrDelete(bvalue4->getName());      // Value name
        name4 = name4.substring(0, 6);                  // String length limit for value name
        double value4 = bvalue4->value;                 // Value as double in SI unit
        bool valid4 = bvalue4->valid;                   // Valid information 
        String svalue4 = formatValue(bvalue4, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit4 = formatValue(bvalue4, commonData).unit;        // Unit of value
        if(valid4 == true){
            svalue4old = svalue4;   	                // Save old value
            unit4old = unit4;                           // Save old unit
        }

        // Get boat values DBT
        GwApi::BoatValue *bvalue5 = pageData.values[4]; // Second element in list (only one value by PageOneValue)
        String name5 = xdrDelete(bvalue5->getName());      // Value name
        name5 = name5.substring(0, 6);                  // String length limit for value name
        double value5 = bvalue5->value;                 // Value as double in SI unit
        bool valid5 = bvalue5->valid;                   // Valid information 
        String svalue5 = formatValue(bvalue5, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit5 = formatValue(bvalue5, commonData).unit;        // Unit of value
        if(valid5 == true){
            svalue5old = svalue5;   	                // Save old value
            unit5old = unit5;                           // Save old unit
        }

        // Get boat values STW
        GwApi::BoatValue *bvalue6 = pageData.values[5]; // Second element in list (only one value by PageOneValue)
        String name6 = xdrDelete(bvalue6->getName());      // Value name
        name6 = name6.substring(0, 6);                  // String length limit for value name
        double value6 = bvalue6->value;                 // Value as double in SI unit
        bool valid6 = bvalue6->valid;                   // Valid information 
        String svalue6 = formatValue(bvalue6, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit6 = formatValue(bvalue6, commonData).unit;        // Unit of value
        if(valid6 == true){
            svalue6old = svalue6;   	                // Save old value
            unit6old = unit6;                           // Save old unit
        }

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageWindRose, %s:%f,  %s:%f,  %s:%f,  %s:%f,  %s:%f,  %s:%f", name1.c_str(), value1, name2.c_str(), value2, name3.c_str(), value3, name4.c_str(), value4, name5.c_str(), value5, name6.c_str(), value6);

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

        // Show values AWA
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(10, 65);
        getdisplay().print(svalue1);                     // Value
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(10, 95);
        getdisplay().print(name1);                       // Name
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(10, 115);
        getdisplay().print(" ");
        if(holdvalues == false){
            getdisplay().print(unit1);                   // Unit
        }
        else{
            getdisplay().print(unit1old);                // Unit
        }

        // Horizintal separator left
        getdisplay().fillRect(0, 149, 60, 3, pixelcolor);

        // Show values AWS
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(10, 270);
        getdisplay().print(svalue2);                     // Value
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(10, 220);
        getdisplay().print(name2);                       // Name
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(10, 190);
        getdisplay().print(" ");
        if(holdvalues == false){
            getdisplay().print(unit2);                   // Unit
        }
        else{
            getdisplay().print(unit2old);                // Unit
        }

        // Show values TWD
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(295, 65);
        if(valid3 == true){
            getdisplay().print(abs(value3 * 180 / PI), 0);   // Value
        }
        else{
            getdisplay().print("---");                   // Value
        }
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(335, 95);
        getdisplay().print(name3);                       // Name
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(335, 115);
        getdisplay().print(" ");
        if(holdvalues == false){
            getdisplay().print(unit3);                   // Unit
        }
        else{
            getdisplay().print(unit3old);                // Unit
        }

        // Horizintal separator right
        getdisplay().fillRect(340, 149, 80, 3, pixelcolor);

        // Show values TWS
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        getdisplay().setCursor(295, 270);
        getdisplay().print(svalue4);                     // Value
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(335, 220);
        getdisplay().print(name4);                       // Name
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(335, 190);
        getdisplay().print(" ");
        if(holdvalues == false){
            getdisplay().print(unit4);                   // Unit
        }
        else{  
            getdisplay().print(unit4old);                // Unit
        }

//*******************************************************************************************
        
        // Draw wind rose
        int rInstrument = 110;     // Radius of grafic instrument
        float pi = 3.141592;

        getdisplay().fillCircle(200, 150, rInstrument + 10, pixelcolor);    // Outer circle
        getdisplay().fillCircle(200, 150, rInstrument + 7, bgcolor);        // Outer circle     
        getdisplay().fillCircle(200, 150, rInstrument - 10, pixelcolor);    // Inner circle
        getdisplay().fillCircle(200, 150, rInstrument - 13, bgcolor);       // Inner circle

        for(int i=0; i<360; i=i+10)
        {
            // Scaling values
            float x = 200 + (rInstrument-30)*sin(i/180.0*pi);  //  x-coordinate dots
            float y = 150 - (rInstrument-30)*cos(i/180.0*pi);  //  y-coordinate cots 
            const char *ii = "";
            switch (i)
            {
            case 0: ii="0"; break;
            case 30 : ii="30"; break;
            case 60 : ii="60"; break;
            case 90 : ii="90"; break;
            case 120 : ii="120"; break;
            case 150 : ii="150"; break;
            case 180 : ii="180"; break;
            case 210 : ii="210"; break;
            case 240 : ii="240"; break;
            case 270 : ii="270"; break;
            case 300 : ii="300"; break;
            case 330 : ii="330"; break;
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

        // Draw wind pointer
        float startwidth = 8;       // Start width of pointer
        if(valid2 == true || holdvalues == true || simulation == true){
            float sinx=sin(value1);     // Wind direction
            float cosx=cos(value1);
            // Normal pointer
            // Pointer as triangle with center base 2*width
            float xx1 = -startwidth;
            float xx2 = startwidth;
            float yy1 = -startwidth;
            float yy2 = -(rInstrument-15); 
            getdisplay().fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),pixelcolor);   
            // Inverted pointer
            // Pointer as triangle with center base 2*width
            float endwidth = 2;         // End width of pointer
            float ix1 = endwidth;
            float ix2 = -endwidth;
            float iy1 = -(rInstrument-15);
            float iy2 = -endwidth;
            getdisplay().fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
                200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
                200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),pixelcolor);
        }

        // Center circle
        getdisplay().fillCircle(200, 150, startwidth + 6, bgcolor);
        getdisplay().fillCircle(200, 150, startwidth + 4, pixelcolor);

//*******************************************************************************************

        // Show values DBT
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic16pt7b);
        getdisplay().setCursor(160, 200);
        getdisplay().print(svalue5);                     // Value
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(190, 215);
        getdisplay().print(" ");
        if(holdvalues == false){
            getdisplay().print(unit5);                   // Unit
        }
        else{  
            getdisplay().print(unit5old);                // Unit
        }

        // Show values STW
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&DSEG7Classic_BoldItalic16pt7b);
        getdisplay().setCursor(160, 130);
        getdisplay().print(svalue6);                     // Value
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(190, 90);
        getdisplay().print(" ");
        if(holdvalues == false){
            getdisplay().print(unit6);                   // Unit
        }
        else{  
            getdisplay().print(unit6old);                // Unit
        }

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
    return new PageWindRose(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageWindRose(
    "WindRose",         // Page name
    createPage,         // Action
    0,                  // Number of bus values depends on selection in Web configuration
    {"AWA", "AWS", "TWD", "TWS", "DBT", "STW"},    // Bus values we need in the page
    true                // Show display header on/off
);

#endif
