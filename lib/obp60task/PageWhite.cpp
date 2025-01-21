#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

#include "Pagedata.h"
#include "OBP60Extensions.h"
#include "MFD_OBP60_400x300_sw.h"       // MFD with logo
#include "Logo_OBP_400x300_sw.h"        // OBP Logo

class PageWhite : public Page
{
char mode = 'W';  // display mode (W)hite | (L)ogo | (M)FD logo

public:
    PageWhite(CommonData &common){
        commonData = &common;
        common.logger->logDebug(GwLog::LOG,"Instantiate PageWhite");
    }

    virtual int handleKey(int key) {
         // Change display mode
        if (key == 1) {
            if (mode == 'W') {
                mode = 'L';
            } else if (mode == 'L') {
                mode = 'M';
            } else {
                mode = 'W';
            }
            return 0;
        }
        return key;
    }

    virtual void displayPage(PageData &pageData){
        GwConfigHandler *config = commonData->config;
        GwLog *logger = commonData->logger;

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

        if (mode == 'L') {
            getdisplay().drawBitmap(0, 0, gImage_Logo_OBP_400x300_sw, getdisplay().width(), getdisplay().height(), commonData->fgcolor);
        } else if (mode == 'M') {
            getdisplay().drawBitmap(0, 0, gImage_MFD_OBP60_400x300_sw, getdisplay().width(), getdisplay().height(), commonData->fgcolor);
        }

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
