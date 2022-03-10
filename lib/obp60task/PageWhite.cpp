#include "Pagedata.h"
#include "OBP60ExtensionPort.h"

class PageWhite : public Page{
    bool keylock = false;               // Keylock

    public:
    PageWhite(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageWhite");
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData){
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;

        // Get config data
        String flashLED = config->getString(config->flashLED);

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setPortPin(OBP_FLASH_LED, false); 
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageWhite");

        // Draw page
        //***********************************************************

        // Set background color
        int bgcolor = GxEPD_WHITE;

        // Clear display by call in obp60task.cpp in main loop

        // Update display
        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, bgcolor);     // Draw white sreen
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)

    };
};

static Page* createPage(CommonData &common){
    return new PageWhite(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageWhite(
    "WhitePage",    // Page name
    createPage,     // Action
    0,              // Number of bus values depends on selection in Web configuration
    false           // Show display header on/off
);