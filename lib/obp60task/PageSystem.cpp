#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

/*
 * Special system page, called directly with fast key sequence 5,4
 * Out of normal page order.
 */

class PageSystem : public Page{
    bool keylock = false;

    public:
    PageSystem(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageSystem");
    }

    virtual int handleKey(int key){
        // Code for keylock
        if (key == 11) {
            keylock = !keylock;
            return 0;
        }
        return key;
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData){
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;

        // Get config data
        String displaycolor = config->getString(config->displaycolor);
        String backlightMode = config->getString(config->backlight);
        String flashLED = config->getString(config->flashLED);

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageSystem");

        // Draw page
        //***********************************************************

        // Set colors
        int fgcolor = GxEPD_BLACK;
        int bgcolor = GxEPD_WHITE;
        if (displaycolor != "Normal") {
            fgcolor = GxEPD_WHITE;
            bgcolor = GxEPD_BLACK;
        }

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 60);
        getdisplay().print("System Information and Settings");

        // Key Layout
        getdisplay().setTextColor(fgcolor);
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if (keylock == false) {
            getdisplay().setCursor(10, 290);
            getdisplay().print("[STBY]");
            if (String(backlightMode) == "Control by Key") {
                getdisplay().setCursor(343, 290);
                getdisplay().print("[ILUM]");
            }
        }
        else {
            getdisplay().setCursor(130, 290);
            getdisplay().print(" [    Keylock active    ]");
        }

        // Update display
        getdisplay().nextPage();    // Partial update (fast)

    };
};

static Page* createPage(CommonData &common){
    return new PageSystem(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageSystem(
    "System",   // Page name
    createPage, // Action
    0,          // No bus values
    true        // Headers are anabled so far
);

#endif
