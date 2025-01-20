#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

class PageTwoValues : public Page
{
    public:
    PageTwoValues(CommonData &common){
        commonData = &common;
        common.logger->logDebug(GwLog::LOG,"Instantiate PageTwoValue");
    }

    virtual int handleKey(int key){
        // Code for keylock
        if(key == 11){
            commonData->keylock = !commonData->keylock;
            return 0;                   // Commit the key
        }
        return key;
    }

    virtual void displayPage(PageData &pageData){
        GwConfigHandler *config = commonData->config;
        GwLog *logger = commonData->logger;

        // Old values for hold function
        static String svalue1old = "";
        static String unit1old = "";
        static String svalue2old = "";
        static String unit2old = "";

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
        String svalue1 = formatValue(bvalue1, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, *commonData).unit;        // Unit of value

        // Get boat values #2
        GwApi::BoatValue *bvalue2 = pageData.values[1]; // Second element in list (only one value by PageOneValue)
        String name2 = xdrDelete(bvalue2->getName());   // Value name
        name2 = name2.substring(0, 6);                  // String length limit for value name
        double value2 = bvalue2->value;                 // Value as double in SI unit
        bool valid2 = bvalue2->valid;                   // Valid information 
        String svalue2 = formatValue(bvalue2, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit2 = formatValue(bvalue2, *commonData).unit;        // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageTwoValues, %s: %f, %s: %f", name1.c_str(), value1, name2.c_str(), value2);

        // Draw page
        //***********************************************************

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        // ############### Value 1 ################

        // Show name
        getdisplay().setTextColor(commonData->fgcolor);
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 80);
        getdisplay().print(name1);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 130);
        if(holdvalues == false){
            getdisplay().print(unit1);                       // Unit
        }
        else{
            getdisplay().print(unit1old);
        }

        // Switch font if format for any values
        if(bvalue1->getFormat() == "formatLatitude" || bvalue1->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(50, 130);
        }
        else if(bvalue1->getFormat() == "formatTime" || bvalue1->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(170, 105);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic42pt7b);
            getdisplay().setCursor(180, 130);
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
        getdisplay().fillRect(0, 145, 400, 3, commonData->fgcolor);

        // ############### Value 2 ################

        // Show name
        getdisplay().setFont(&Ubuntu_Bold20pt7b);
        getdisplay().setCursor(20, 190);
        getdisplay().print(name2);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 240);
        if(holdvalues == false){
            getdisplay().print(unit2);                       // Unit
        }
        else{
            getdisplay().print(unit2old);
        }

        // Switch font if format for any values
        if(bvalue2->getFormat() == "formatLatitude" || bvalue2->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(50, 240);
        }
        else if(bvalue2->getFormat() == "formatTime" || bvalue2->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold20pt7b);
            getdisplay().setCursor(170, 215);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic42pt7b);
            getdisplay().setCursor(180, 240);
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

        // Update display
        getdisplay().nextPage();    // Partial update (fast)
    };
};

static Page *createPage(CommonData &common){
    return new PageTwoValues(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageTwoValues(
    "TwoValues",    // Page name
    createPage,     // Action
    2,              // Number of bus values depends on selection in Web configuration
    true            // Show display header on/off
);

#endif
