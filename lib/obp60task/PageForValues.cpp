#include "Pagedata.h"
#include "OBP60ExtensionPort.h"

class PageForValues : public Page
{
    bool keylock = false;               // Keylock

    public:
    PageForValues(CommonData &comon){
        comon.logger->logDebug(GwLog::LOG,"Show PageForValues");
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
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool refresh = config->getBool(config->refresh);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);
        
        // Get boat values #1
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = bvalue1->getName().c_str();      // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        double value1 = bvalue1->value;                 // Value as double in SI unit
        bool valid1 = bvalue1->valid;                   // Valid information 
        String svalue1 = formatValue(bvalue1, commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, commonData).unit;        // Unit of value

        // Get boat values #2
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // Second element in list (only one value by PageOneValue)
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
        LOG_DEBUG(GwLog::LOG,"Drawing at PageForValues, %s: %f, %s: %f, %s: %f, %s: %f", name1, value1, name2, value2, name3, value3, name4, value4);

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

        // ############### Value 1 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold16pt7b);
        display.setCursor(20, 45);
        display.print(name1);                           // Page name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(20, 65);
        if(holdvalues == false){
            display.print(unit1);                       // Unit
        }
        else{
            display.print(unit1old);
        }

        // Switch font if format for any values
        if(bvalue1->getFormat() == "formatLatitude" || bvalue1->getFormat() == "formatLongitude"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(120, 55);
        }
        else if(bvalue1->getFormat() == "formatTime" || bvalue1->getFormat() == "formatDate"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(150, 58);
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(180, 65);
        }

        // Show bus data
        if(holdvalues == false){
            display.print(svalue1);                                     // Real value as formated string
        }
        else{
            display.print(svalue1old);                                  // Old value as formated string
        }
        if(valid1 == true){
            svalue1old = svalue1;                                       // Save the old value
            unit1old = unit1;                                           // Save the old unit
        }

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        display.fillRect(0, 80, 400, 3, pixelcolor);

        // ############### Value 2 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold16pt7b);
        display.setCursor(20, 113);
        display.print(name2);                           // Page name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(20, 133);
        if(holdvalues == false){
            display.print(unit2);                       // Unit
        }
        else{
            display.print(unit2old);
        }

        // Switch font if format for any values
        if(bvalue2->getFormat() == "formatLatitude" || bvalue2->getFormat() == "formatLongitude"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(120, 123);
        }
        else if(bvalue2->getFormat() == "formatTime" || bvalue2->getFormat() == "formatDate"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(150, 123);
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(180, 133);
        }

        // Show bus data
        if(holdvalues == false){
            display.print(svalue2);                                     // Real value as formated string
        }
        else{
            display.print(svalue2old);                                  // Old value as formated string
        }
        if(valid2 == true){
            svalue2old = svalue2;                                       // Save the old value
            unit2old = unit2;                                           // Save the old unit
        }

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        display.fillRect(0, 146, 400, 3, pixelcolor);

        // ############### Value 3 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold16pt7b);
        display.setCursor(20, 181);
        display.print(name3);                           // Page name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(20, 201);
        if(holdvalues == false){
            display.print(unit3);                       // Unit
        }
        else{
            display.print(unit3old);
        }

        // Switch font if format for any values
        if(bvalue3->getFormat() == "formatLatitude" || bvalue3->getFormat() == "formatLongitude"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(120, 191);
        }
        else if(bvalue3->getFormat() == "formatTime" || bvalue3->getFormat() == "formatDate"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(150, 191);
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(180, 201);
        }

        // Show bus data
        if(holdvalues == false){
            display.print(svalue3);                                     // Real value as formated string
        }
        else{
            display.print(svalue3old);                                  // Old value as formated string
        }
        if(valid3 == true){
            svalue3old = svalue3;                                       // Save the old value
            unit3old = unit3;                                           // Save the old unit
        }

        // ############### Horizontal Line ################

        // Horizontal line 3 pix
        display.fillRect(0, 214, 400, 3, pixelcolor);

        // ############### Value 4 ################

        // Show name
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold16pt7b);
        display.setCursor(20, 249);
        display.print(name4);                           // Page name

        // Show unit
        display.setTextColor(textcolor);
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(20, 269);
        if(holdvalues == false){
            display.print(unit4);                       // Unit
        }
        else{
            display.print(unit4old);
        }

        // Switch font if format for any values
        if(bvalue4->getFormat() == "formatLatitude" || bvalue4->getFormat() == "formatLongitude"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(120, 259);
        }
        else if(bvalue4->getFormat() == "formatTime" || bvalue4->getFormat() == "formatDate"){
            display.setFont(&Ubuntu_Bold12pt7b);
            display.setCursor(150, 259);
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic20pt7b);
            display.setCursor(180, 269);
        }

        // Show bus data
        if(holdvalues == false){
            display.print(svalue4);                                     // Real value as formated string
        }
        else{
            display.print(svalue4old);                                  // Old value as formated string
        }
        if(valid4 == true){
            svalue4old = svalue4;                                       // Save the old value
            unit4old = unit4;                                           // Save the old unit
        }


        // ############### Key Layout ################

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
    return new PageForValues(common);
}/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageForValues(
    "forValues",    // Page name
    createPage,     // Action
    4,              // Number of bus values depends on selection in Web configuration
    true            // Show display header on/off
);