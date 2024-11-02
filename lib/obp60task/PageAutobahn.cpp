#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

/*
  Autobahn view
    XTE - Cross track error
    COG - Track / Course over ground
    DTW - Distance to waypoint
    BTW - Bearing to waypoint
*/

#define ship_width 32
#define ship_height 32
static unsigned char ship_bits[] PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x03, 0x00,
   0x00, 0xc0, 0x07, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0xe0, 0x0f, 0x00,
   0x00, 0xe0, 0x0f, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00, 0xf0, 0x1f, 0x00,
   0x00, 0xf0, 0x1f, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xfc, 0x7f, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0xfc, 0x7f, 0x00,
   0x00, 0xfc, 0x7f, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0xfc, 0x7f, 0x00,
   0x00, 0xfc, 0x7f, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00,
   0x00, 0xf0, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 };

class PageAutobahn : public Page{
    bool keylock = false;               // Keylock

    public:
    PageAutobahn(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageAutobahn");
    }

    void drawSegment(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                     uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3,
                     uint16_t color, bool fill){
       if (fill == true) {
           // no primitive for quadrangular object
           // we create it from 2 triangles
           getdisplay().fillTriangle(x0, y0, x1, y1, x3, y3, color);
           getdisplay().fillTriangle(x1, y1, x2, y2, x3, y3, color);
       } else {
           // draw outline
           getdisplay().drawLine(x0, y0, x1, y1, color);
           getdisplay().drawLine(x1, y1, x2, y2, color);
             getdisplay().drawLine(x2, y2, x3, y3, color);
           getdisplay().drawLine(x3, y3, x0, y0, color);
       }
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

        // Get config data
        String flashLED = config->getString(config->flashLED);
        String displaycolor = config->getString(config->displaycolor);
        String backlightMode = config->getString(config->backlight);

        String trackStep = config->getString(config->trackStep);
        double seg_deg = trackStep.toFloat(); // degrees per display segment

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false);
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageAutobahn");

        // Draw page
        //***********************************************************

        // Set background color and text color
        int textcolor = GxEPD_BLACK;
        int pixelcolor = GxEPD_BLACK;
        int bgcolor = GxEPD_WHITE;
        if(displaycolor != "Normal"){
            textcolor = GxEPD_WHITE;
            pixelcolor = GxEPD_WHITE;
            bgcolor = GxEPD_BLACK;
        }
        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        // descriptions
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        getdisplay().setCursor(50, 188);
        getdisplay().print("Cross-track error");
        getdisplay().setCursor(270, 188);
        getdisplay().print("Track");
        getdisplay().setCursor(45, 275);
        getdisplay().print("Distance to waypoint");
        getdisplay().setCursor(260, 275);
        getdisplay().print("Bearing");

        // values
        getdisplay().setFont(&DSEG7Classic_BoldItalic30pt7b);

        int16_t  x, y;
        uint16_t w, h;

        GwApi::BoatValue *bv_xte = pageData.values[0]; // XTE
        String sval_xte = formatValue(bv_xte, commonData).svalue;
        getdisplay().getTextBounds(sval_xte, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(160-w, 170);
        getdisplay().print(sval_xte);

        GwApi::BoatValue *bv_cog = pageData.values[1]; // COG
        String sval_cog = formatValue(bv_cog, commonData).svalue;
        getdisplay().getTextBounds(sval_cog, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(360-w, 170);
        getdisplay().print(sval_cog);

        GwApi::BoatValue *bv_dtw = pageData.values[2]; // DTW
        String sval_dtw = formatValue(bv_dtw, commonData).svalue;
        getdisplay().getTextBounds(sval_dtw, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(160-w, 257);
        getdisplay().print(sval_dtw);

        GwApi::BoatValue *bv_btw = pageData.values[3]; // BTW
        String sval_btw = formatValue(bv_btw, commonData).svalue;
        getdisplay().getTextBounds(sval_btw, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(360-w, 257);
        getdisplay().print(sval_btw);

        // autobahn view

        // draw ship symbol (as bitmap)
        getdisplay().drawXBitmap(184, 68, ship_bits, ship_width, ship_height, pixelcolor);

        // draw next waypoint name
        String sval_wpname = "Tonne 122";
        getdisplay().setFont(&Ubuntu_Bold10pt7b);
        getdisplay().getTextBounds(sval_wpname, 0, 150, &x, &y, &w, &h);
        // TODO if text don't fix use smaller font size.
        // if smallest size does not fit use 2 lines
        // last resort: clip with ellipsis
        getdisplay().setCursor(200 - w / 2, 60);

        getdisplay().print(sval_wpname);

        // draw course segments

        double diff = bv_cog->value - bv_btw->value;
        if (diff < -180) {
            diff += 360;
        } else if (diff > 180:) {
            diff -= 360
        }

        // default all segments activated
        bool seg[6] = {true, true, true, true, true, true};
        // number of inactive segments
        int nseg = std::min(std::floor(std::abs(diff) / seg_deg), 5);

        int order[6];
        if (diff < 0) {
            // right
            order[0] = 6; order[1] = 5; order[2] = 4;
            order[3] = 1; order[4] = 2; order[5] = 3;
        else if (diff > 0) {
            // left
            order[0] = 3; order[1] = 2; order[2] = 1;
            order[3] = 4; order[4] = 5; order[5] = 6;
        }
        int i = 0;
        while (nseg > 0) {
            seg[order[i]] = false;
            i += 1;
            nseg -= 1;
        }

        // left segments
        drawSegment(0, 54, 46, 18, 75, 18, 0, 90, pixelcolor, seg[0]);
        drawSegment(0, 100, 82, 18, 112, 18, 50, 100, pixelcolor, seg[1]);
        drawSegment(60, 100, 117, 18, 147, 18, 110, 100, pixelcolor,seg[2]);
        // right segments
        drawSegment(399, 54, 354, 18, 325, 18, 399, 90, pixelcolor, seg[3]);
        drawSegment(399, 100, 318, 18, 289, 18, 350, 100, pixelcolor, seg[4]));
        drawSegment(340, 100, 283, 18, 253, 18, 290, 100, pixelcolor, seg[5]);

        // Key Layout
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            getdisplay().setCursor(130, 296);
            getdisplay().print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                getdisplay().setCursor(343, 296);
                getdisplay().print("[ILUM]");
            }
        }
        else{
            getdisplay().setCursor(130, 296);
            getdisplay().print(" [    Keylock active    ]");
        }

        // Update display
        getdisplay().nextPage();    // Partial update (fast)

    };
};

static Page* createPage(CommonData &common){
    return new PageAutobahn(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageAutobahn(
    "Autobahn",            // Page name
    createPage,             // Action
    0,                      // Number of bus values depends on selection in Web configuration
    {"XTE", "COG", "DTW", "BTW"}, // Bus values we need in the page
    true                    // Show display header on/off
);

#endif
