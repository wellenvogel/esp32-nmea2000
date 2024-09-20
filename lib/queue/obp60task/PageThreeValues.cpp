#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageThreeValues : public Page
{
    bool keylock = false;               // Keylock

    public:
    PageThreeValues(CommonData &comon){
        comon.logger->logDebug(GwLog::LOG,"Show PageThreeValue");
    }

    virtual int handleKey(int key){
        if(key == 11){                  // Code for keylock
            keylock = !keylock;         // Toggle keylock
            return 0;                   // Commit the key
        }
        return key;
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData){
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;

        // Old values for hold function
        static String svalue1old = "";
        static String unit1old = "";
        static String svalue2old = "";
        static String unit2old = "";
        static String svalue3old = "";
        static String unit3old = "";

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        // bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);
        
        // Get boat values #1
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = xdrDelete(bvalue1->getName());   // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        double value1 = bvalue1->value;                 // Value as double in SI unit
        bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, commonData).unit;        // Unit of value

        // Get boat values #2
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // Second element in list (only one value by PageOneValue)
        String name2 = xdrDelete(bvalue2->getName());   // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        double value2 = bvalue2->value;                 // Value as double in SI unit
        bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, commonData).unit;        // Unit of value

        // Get boat values #3
        GwApi::BoatValue *bvalue3 = pageData.values[2]; // Second element in list (only one value by PageOneValue)
        String name3 = xdrDelete(bvalue3->getName());      // Value name
        name3 = name3.substring(0, 6);                  // String length limit for value name
        double value3 = bvalue3->value;                 // Value as double in SI unit
        bool valid3 = bvalue3->valid;                   // Valid information 
        String svalue3 = formatValue(bvalue3, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit3 = formatValue(bvalue3, commonData).unit;        // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageThreeValues, %s: %f, %s: %f, %s: %f", name1.c_str(), value1, name2.c_str(), value2, name3.c_str(), value3);

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
        /// Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        // ############### Value 1 ################

        // Show name
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 55);
        getdisplay().print(name1);                           // Page name

        // Show unit
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 90);
        if(holdvalues == false){
            getdisplay().print(unit1);                       // Unit
        }
        else{
            getdisplay().print(unit1old);
        }

        // Switch font if format for any values
        if(bvalue1->getFormat() == "formatLatitude" || bvalue1->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(50, 90);
        }
        else if(bvalue1->getFormat() == "formatTime" || bvalue1->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(170, 68);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic30pt7b);
            getdisplay().setCursor(180, 90);
        }

        // Show bus data
        if(holdvalues == false){
            getdisplay().print(svalue1);                                     // Real value as formated string
        }
        else{
            getdisplay().print(svalue1old);                                  // Old value as formated string
        }
        if(valid1 == true){
            svalue1old = svalue1;                                       // Save the old value
            unit1old = unit1;                                           // Save the old unit
        }

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        getdisplay().fillRect(0, 105, 400, 3, pixelcolor);

        // ############### Value 2 ################

        // Show name
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 145);
        getdisplay().print(name2);                           // Page name

        // Show unit
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 180);
        if(holdvalues == false){
            getdisplay().print(unit2);                       // Unit
        }
        else{
            getdisplay().print(unit2old);
        }

        // Switch font if format for any values
        if(bvalue2->getFormat() == "formatLatitude" || bvalue2->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(50, 180);
        }
        else if(bvalue2->getFormat() == "formatTime" || bvalue2->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(170, 158);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic30pt7b);
            getdisplay().setCursor(180, 180);
        }

        // Show bus data
        if(holdvalues == false){
            getdisplay().print(svalue2);                                     // Real value as formated string
        }
        else{
            getdisplay().print(svalue2old);                                  // Old value as formated string
        }
        if(valid2 == true){
            svalue2old = svalue2;                                       // Save the old value
            unit2old = unit2;                                           // Save the old unit
        }

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        getdisplay().fillRect(0, 195, 400, 3, pixelcolor);

        // ############### Value 3 ################

        // Show name
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 235);
        getdisplay().print(name3);                           // Page name

        // Show unit
        getdisplay().setTextColor(textcolor);
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 270);
        if(holdvalues == false){
            getdisplay().print(unit3);                       // Unit
        }
        else{
            getdisplay().print(unit3old);
        }

        // Switch font if format for any values
        if(bvalue3->getFormat() == "formatLatitude" || bvalue3->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(50, 270);
        }
        else if(bvalue3->getFormat() == "formatTime" || bvalue3->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(170, 248);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic30pt7b);
            getdisplay().setCursor(180, 270);
        }

        // Show bus data
        if(holdvalues == false){
            getdisplay().print(svalue3);                                     // Real value as formated string
        }
        else{
            getdisplay().print(svalue3old);                                  // Old value as formated string
        }
        if(valid3 == true){
            svalue3old = svalue3;                                       // Save the old value
            unit3old = unit3;                                           // Save the old unit
        }


        // ############### Key Layout ################

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
    return new PageThreeValues(common);
}/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageThreeValues(
    "ThreeValues",  // Page name
    createPage,     // Action
    3,              // Number of bus values depends on selection in Web configuration
    true            // Show display header on/off
);

#endif