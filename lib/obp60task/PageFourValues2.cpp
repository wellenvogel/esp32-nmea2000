#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageFourValues2 : public Page
{
    bool keylock = false;               // Keylock

    public:
    PageFourValues2(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageFourValues2");
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
        static String svalue4old = "";
        static String unit4old = "";

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        // bool simulation = config->getBool(config->useSimuData);
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
        String name3 = xdrDelete(bvalue3->getName());   // Value name
        name3 = name3.substring(0, 6);                  // String length limit for value name
        double value3 = bvalue3->value;                 // Value as double in SI unit
        bool valid3 = bvalue3->valid;                   // Valid information 
        String svalue3 = formatValue(bvalue3, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit3 = formatValue(bvalue3, commonData).unit;        // Unit of value

        // Get boat values #4
        GwApi::BoatValue *bvalue4 = pageData.values[3]; // Second element in list (only one value by PageOneValue)
        String name4 = xdrDelete(bvalue4->getName());   // Value name
        name4 = name4.substring(0, 6);                  // String length limit for value name
        double value4 = bvalue4->value;                 // Value as double in SI unit
        bool valid4 = bvalue4->valid;                   // Valid information 
        String svalue4 = formatValue(bvalue4, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit4 = formatValue(bvalue4, commonData).unit;        // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageFourValues2, %s: %f, %s: %f, %s: %f, %s: %f", name1.c_str(), value1, name2.c_str(), value2, name3.c_str(), value3, name4.c_str(), value4);

        // Draw page
        //***********************************************************

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        getdisplay().setTextColor(commonData.fgcolor);

        // ############### Value 1 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 55);
        getdisplay().print(name1);                           // Page name

        // Show unit
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
            getdisplay().setFont(&Ubuntu_Bold12pt7b);
            getdisplay().setCursor(100, 90);
        }
        else if(bvalue1->getFormat() == "formatTime" || bvalue1->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold12pt7b);
            getdisplay().setCursor(180, 77);
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
        getdisplay().fillRect(0, 105, 400, 3, commonData.fgcolor);

        // ############### Value 2 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 145);
        getdisplay().print(name2);                           // Page name

        // Show unit
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
            getdisplay().setFont(&Ubuntu_Bold12pt7b);
            getdisplay().setCursor(100, 180);
        }
        else if(bvalue2->getFormat() == "formatTime" || bvalue2->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold12pt7b);
            getdisplay().setCursor(180, 158);
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
        getdisplay().fillRect(0, 195, 400, 3, commonData.fgcolor);

        // ############### Value 3 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 220);
        getdisplay().print(name3);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(20, 240);
        if(holdvalues == false){
            getdisplay().print(unit3);                       // Unit
        }
        else{
            getdisplay().print(unit3old);
        }

        // Switch font if format for any values
        if(bvalue3->getFormat() == "formatLatitude" || bvalue3->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold8pt7b);
            getdisplay().setCursor(50, 240);
        }
        else if(bvalue3->getFormat() == "formatTime" || bvalue3->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold8pt7b);
            getdisplay().setCursor(100, 240);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
            getdisplay().setCursor(80, 270);
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

        // ############### Vertical Line ################

        // Vertical line 3 pix
        getdisplay().fillRect(200, 195, 3, 75, commonData.fgcolor);

        // ############### Value 4 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(220, 220);
        getdisplay().print(name4);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(220, 240);
        if(holdvalues == false){
            getdisplay().print(unit4);                       // Unit
        }
        else{
            getdisplay().print(unit4old);
        }

        // Switch font if format for any values
        if(bvalue4->getFormat() == "formatLatitude" || bvalue4->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold8pt7b);
            getdisplay().setCursor(250, 240);
        }
        else if(bvalue4->getFormat() == "formatTime" || bvalue4->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold8pt7b);
            getdisplay().setCursor(300, 240);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
            getdisplay().setCursor(280, 270);
        }

        // Show bus data
        if(holdvalues == false){
            getdisplay().print(svalue4);                                     // Real value as formated string
        }
        else{
            getdisplay().print(svalue4old);                                  // Old value as formated string
        }
        if(valid4 == true){
            svalue4old = svalue4;                                       // Save the old value
            unit4old = unit4;                                           // Save the old unit
        }


        // ############### Key Layout ################

        // Key Layout
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
    return new PageFourValues2(common);
}/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageFourValues2(
    "FourValues2",        // Page name
    createPage,         // Action
    4,                  // Number of bus values depends on selection in Web configuration
    true                // Show display header on/off
);

#endif
