#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

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
            setFlashLED(false); 
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageWhite");

        // Draw page
        //***********************************************************

        // Set background color
        int bgcolor = GxEPD_WHITE;

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        // Update display
        getdisplay().nextPage();    // Partial update (fast)

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

#endif