#include "Pagedata.h"
#include "OBP60ExtensionPort.h"

class PageOneValue : public Page{
    public:
    virtual void displayPage(CommonData &commonData, PageData &pageData){
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;
        
        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        bool simulation = config->getBool(config->useSimuData);
        String displaycolor = config->getString(config->displaycolor);
        bool holdvalues = config->getBool(config->holdvalues);
        
        // Get boat values
        GwApi::BoatValue *bvalue=pageData.values[0];    // First element in list (only one value by PageOneValue)
        String name1 = bvalue->getName().c_str();       // Value name
        double value1 = bvalue->value;                  // Value as double in SI unit
        String svalue1 = formatValue(bvalue).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue).unit;        // Unit of value
        bool valid1 = bvalue->valid;                    // Valid flag of value

        // Logging boat values
        if (bvalue == NULL) return;
        LOG_DEBUG(GwLog::LOG,"Drawing at PageOneValue, p=%s, v=%f", name1, value1);

        // Draw page
        //***********************************************************

        // Clear display, set background color and text color
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
        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, bgcolor);   // Draw white sreen
        display.setTextColor(textcolor);

        // Show name
        display.setFont(&Ubuntu_Bold32pt7b);
        display.setCursor(20, 100);
        display.print(name1);                           // Page name

        // Show unit
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(270, 100);              
        display.print(unit1);

        // Switch font if format latitude or longitude
        if(bvalue->getFormat() == "formatLatitude" || bvalue->getFormat() == "formatLongitude"){
            display.setFont(&Ubuntu_Bold20pt7b);
            display.setCursor(20, 180);
        }
        else{
            display.setFont(&DSEG7Classic_BoldItalic60pt7b);
            display.setCursor(20, 240);
        }

        // Reading bus data or using simulation data
        if(simulation == true){
            value1 = 84;
            value1 += float(random(0, 120)) / 10;       // Simulation data
            display.print(value1,1);
        }
        else{
            display.print(svalue1);                     // Real value as formated string  
        }

        // Key Layout
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setCursor(115, 290);
        display.print(" [  <<<<<<      >>>>>>  ]");
        display.setCursor(343, 290);
        display.print("[ILUM]");

        // Update display
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)

    };
};

static Page* createPage(CommonData &common){return new PageOneValue();}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageOneValue(
    "oneValue",     // Name of page
    createPage,     // Action
    1,              // Number of bus values depends on selection in Web configuration
    true            // Show display header on/off
);