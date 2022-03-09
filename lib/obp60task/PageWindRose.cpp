#include "Pagedata.h"
#include "OBP60ExtensionPort.h"

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
        // Reduce instrument size
        if(key == 2){               // Code for reduce
            lp = lp - 10;
            if(lp < 10){
                lp = 10;
            }
            return 0;               // Commit the key
        }

        // Enlarge instrument size
        if(key == 5){               // Code for enlarge
            lp = lp + 10;
            if(lp > 80){
                lp = 80;
            }
            return 0;               // Commit the key
        }

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

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);

        // Get boat values for AWS
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = bvalue1->getName().c_str();      // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        double value1 = bvalue1->value;                 // Value as double in SI unit
        bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, commonData).unit;        // Unit of value

        // Get boat values for AWD
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // First element in list (only one value by PageOneValue)
        String name2 = bvalue2->getName().c_str();      // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        double value2 = bvalue2->value;                 // Value as double in SI unit
        bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, commonData).unit;        // Unit of value

        // Get boat values #3
        GwApi::BoatValue *bvalue3 = pageData.values[2]; // Second element in list (only one value by PageOneValue)
        String name3 = bvalue3->getName().c_str();      // Value name
        name3 = name3.substring(0, 6);                  // String length limit for value name
        double value3 = bvalue3->value;                 // Value as double in SI unit
        bool valid3 = bvalue3->valid;                   // Valid information 
        String svalue3 = formatValue(bvalue3, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit3 = formatValue(bvalue3, commonData).unit;        // Unit of value

        // Get boat values #4
        GwApi::BoatValue *bvalue4 = pageData.values[3]; // Second element in list (only one value by PageOneValue)
        String name4 = bvalue4->getName().c_str();      // Value name
        name4 = name4.substring(0, 6);                  // String length limit for value name
        double value4 = bvalue4->value;                 // Value as double in SI unit
        bool valid4 = bvalue4->valid;                   // Valid information 
        String svalue4 = formatValue(bvalue4, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit4 = formatValue(bvalue4, commonData).unit;        // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setPortPin(OBP_FLASH_LED, false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageWindRose, %s:%f,  %s:%f", name1, value1, name2, value2);

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

        // Show values AWA
        display.setTextColor(textcolor);
        if(holdvalues == false){
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(10, 65);
            display.print(svalue1);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(10, 95);
            display.print(name1);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(10, 115);
            display.print(" ");
            display.print(unit1);                       // Unit
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(10, 65);
            display.print(svalue1);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(10, 95);
            display.print(name1);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(10, 115);
            display.print(" ");
            display.print(unit1);                       // Unit
        }

        // Horizintal separator left
        display.fillRect(0, 149, 60, 3, pixelcolor);

        // Show values AWS
        display.setTextColor(textcolor);
        if(holdvalues == false){
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(10, 270);
            display.print(svalue2);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(10, 220);
            display.print(name2);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(10, 190);
            display.print(" ");
            display.print(unit2);                       // Unit
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(10, 270);
            display.print(svalue2);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(10, 220);
            display.print(name2);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(10, 190);
            display.print(" ");
            display.print(unit2);                       // Unit
        }

        // Show values TWD
        display.setTextColor(textcolor);
        if(holdvalues == false){
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(295, 65);
            display.print(svalue3);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(335, 95);
            display.print(name3);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(335, 115);
            display.print(" ");
            display.print(unit3);                       // Unit
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(295, 65);
            display.print(svalue3);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(335, 95);
            display.print(name3);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(335, 115);
            display.print(" ");
            display.print(unit3);                       // Unit
        }

        // Horizintal separator right
        display.fillRect(340, 149, 80, 3, pixelcolor);

        // Show values TWS
        display.setTextColor(textcolor);
        if(holdvalues == false){
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(295, 270);
            display.print(svalue4);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(335, 220);
            display.print(name4);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(335, 190);
            display.print(" ");
            display.print(unit4);                       // Unit
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(295, 270);
            display.print(svalue4);                     // Value
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(335, 220);
            display.print(name4);                       // Name
            display.setFont(&Ubuntu_Bold8pt7b);
            display.setCursor(335, 190);
            display.print(" ");
            display.print(unit4);                       // Unit
        }

//*******************************************************************************************
        
        // Draw wind rose
        int rWindGraphic = 110;     // Radius of grafic instrument
        float pi = 3.141592;

        display.fillCircle(200, 150, rWindGraphic + 10, pixelcolor);    // Outer circle
        display.fillCircle(200, 150, rWindGraphic + 7, bgcolor);        // Outer circle     
        display.fillCircle(200, 150, rWindGraphic - 10, pixelcolor);    // Inner circle
        display.fillCircle(200, 150, rWindGraphic - 13, bgcolor);       // Inner circle

        for(int i=0; i<360; i=i+10)
        {
            // Scaling values
            float x = 200 + (rWindGraphic-30)*sin(i/180.0*pi);  //  x-coordinate dots
            float y = 150 - (rWindGraphic-30)*cos(i/180.0*pi);  //  y-coordinate cots 
            const char *ii;
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
            display.getTextBounds(ii, int(x), int(y), &x1, &y1, &w, &h); // Calc width of new string
            display.setCursor(x-w/2, y+h/2);
            if(i % 30 == 0){
                display.setFont(&Ubuntu_Bold8pt7b);
                display.print(ii);
            }

            // Draw sub scale with dots
            float x1c = 200 + rWindGraphic*sin(i/180.0*pi);
            float y1c = 150 - rWindGraphic*cos(i/180.0*pi);
            display.fillCircle((int)x1c, (int)y1c, 2, pixelcolor);
            float sinx=sin(i/180.0*pi);
            float cosx=cos(i/180.0*pi); 

            // Draw sub scale with lines (two triangles)
            if(i % 30 == 0){
                float dx=2;   // Line thickness = 2*dx+1
                float xx1 = -dx;
                float xx2 = +dx;
                float yy1 =  -(rWindGraphic-10);
                float yy2 =  -(rWindGraphic+10);
                display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
                        200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),pixelcolor);
                display.fillTriangle(200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
                        200+(int)(cosx*xx1-sinx*yy2),150+(int)(sinx*xx1+cosx*yy2),
                        200+(int)(cosx*xx2-sinx*yy2),150+(int)(sinx*xx2+cosx*yy2),pixelcolor);  
            }
        }

        // Draw wind pointer
        float sinx=sin(value2);     // Wind direction
        float cosx=cos(value2);
        // Normal pointer
        // Pointer as triangle with center base 2*width
        float startwidth = 8;       // Start width of pointer
        float xx1 = -startwidth;
        float xx2 = startwidth;
        float yy1 = -startwidth;
        float yy2 = -(rWindGraphic-15); 
        display.fillTriangle(200+(int)(cosx*xx1-sinx*yy1),150+(int)(sinx*xx1+cosx*yy1),
            200+(int)(cosx*xx2-sinx*yy1),150+(int)(sinx*xx2+cosx*yy1),
            200+(int)(cosx*0-sinx*yy2),150+(int)(sinx*0+cosx*yy2),pixelcolor);   
        // Inverted pointer
        // Pointer as triangle with center base 2*width
        float endwidth = 2;         // End width of pointer
        float ix1 = endwidth;
        float ix2 = -endwidth;
        float iy1 = -(rWindGraphic-15);
        float iy2 = -endwidth;
        display.fillTriangle(200+(int)(cosx*ix1-sinx*iy1),150+(int)(sinx*ix1+cosx*iy1),
            200+(int)(cosx*ix2-sinx*iy1),150+(int)(sinx*ix2+cosx*iy1),
            200+(int)(cosx*0-sinx*iy2),150+(int)(sinx*0+cosx*iy2),pixelcolor);

        // Center circle
        display.fillCircle(200, 150, startwidth + 4, pixelcolor);

//*******************************************************************************************
        // Key Layout
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(115, 290);
        if(keylock == false){
            display.print(" [  <<<<<<      >>>>>>  ]");
            if(String(backlightMode) == "Control by Key"){              // Key for illumination
                display.setCursor(343, 290);
                display.print("[ILUM]");
            }
        }
        else{
            display.print(" [    Keylock active    ]");
        }

        // Update display
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)

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
    {"AWA", "AWS", "TWD", "TWS"},    // Bus values we need in the page
    true                // Show display header on/off
);