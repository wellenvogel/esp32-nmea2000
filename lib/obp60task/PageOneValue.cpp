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
        bool holdvalues = config->getBool(config->holdvalues);
        
        // Get boat values
        GwApi::BoatValue *value=pageData.values[0];
        String name1 = value->getName().c_str();
        double value1 = value->value;
        bool valid1 = value->valid;
        String format1 = value->getFormat().c_str();

        // Logging boat values
        if (value == NULL) return;
        LOG_DEBUG(GwLog::LOG,"drawing at PageOneValue, p=%s, v=%f", name1, value1);

        // Draw page
        //***********************************************************

        // Clear display
        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);   // Draw white sreen

        // Show name
        display.setFont(&Ubuntu_Bold32pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(20, 100);
        display.print(name1);                           // Page name
        display.setFont(&Ubuntu_Bold20pt7b);
        display.setCursor(270, 100);
        // Show unit        
        if(String(lengthformat) == "m"){
            display.print("m");
        }
        if(String(lengthformat) == "ft"){
            display.print("ft");
        }      
        display.setFont(&DSEG7Classic_BoldItalic60pt7b);
        display.setCursor(20, 240);

        // Reading bus data or using simulation data
        if(simulation == true){
            value1 = 84;
            value1 += float(random(0, 120)) / 10;       // Simulation data
            display.print(value1,1);
        }
        else{
            // Check vor valid real data, display also if hold values activated
            if(valid1 == true || holdvalues == true){
                // Unit conversion
                if(String(lengthformat) == "m"){
                    value1 = value1;                    // Real bus data m
                }
                if(String(lengthformat) == "ft"){
                    value1 = value1 * 3.28084;          // Bus data in ft
                }
                // Resolution switching
                if(value1 <= 99.9){
                    display.print(value1,1);
                }
                else{
                    display.print(value1,0);
                }
            }
            else{
            display.print("---");                       // Missing bus data
            }  
        }

        // Key Layout
        display.setFont(&Ubuntu_Bold8pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(0, 290);
        display.print(" [  <  ]");
        display.setCursor(290, 290);
        display.print("[  >  ]");
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
    "oneValue",
    createPage,
    1
);