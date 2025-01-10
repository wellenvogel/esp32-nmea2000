#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

/*
  XTETrack view
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

class PageXTETrack : public Page
{
    public:
    PageXTETrack(CommonData &common){
        commonData = &common;
        common.logger->logDebug(GwLog::LOG,"Instantiate PageXTETrack");
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

        // Get config data
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);

        String trackStep = config->getString(config->trackStep);
        double seg_step = trackStep.toFloat() * PI / 180;

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false);
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageXTETrack");

        // Draw page
        //***********************************************************

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        getdisplay().setTextColor(commonData->fgcolor);

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
        String sval_xte = formatValue(bv_xte, *commonData).svalue;
        getdisplay().getTextBounds(sval_xte, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(160-w, 170);
        getdisplay().print(sval_xte);

        GwApi::BoatValue *bv_cog = pageData.values[1]; // COG
        String sval_cog = formatValue(bv_cog, *commonData).svalue;
        getdisplay().getTextBounds(sval_cog, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(360-w, 170);
        getdisplay().print(sval_cog);

        GwApi::BoatValue *bv_dtw = pageData.values[2]; // DTW
        String sval_dtw = formatValue(bv_dtw, *commonData).svalue;
        getdisplay().getTextBounds(sval_dtw, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(160-w, 257);
        getdisplay().print(sval_dtw);

        GwApi::BoatValue *bv_btw = pageData.values[3]; // BTW
        String sval_btw = formatValue(bv_btw, *commonData).svalue;
        getdisplay().getTextBounds(sval_btw, 0, 0, &x, &y, &w, &h);
        getdisplay().setCursor(360-w, 257);
        getdisplay().print(sval_btw);

        bool valid = bv_cog->valid && bv_btw->valid;

        // XTETrack view

        // draw ship symbol (as bitmap)
        getdisplay().drawXBitmap(184, 68, ship_bits, ship_width, ship_height, commonData->fgcolor);

        // draw next waypoint name
        String sval_wpname = "no data";

        if (valid) {
			sval_wpname = "Tonne 122";
        }

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
        } else if (diff > 180) {
            diff -= 360;
        }

        // as default all segments activated if valid calculation
        // values are available
        bool seg[6]; // segment layout: [2][1][0] | [3][4][5]
        for (int i=0; i<6; i++) {
            seg[i] = valid;
        }

        // number of inactive segments
        int nseg = std::min(static_cast<int>(std::floor(std::abs(diff) / seg_step)), 5);

        int order[6];
        if (diff > 0) {
            // right
            order[0] = 3; order[1] = 4; order[2] = 5;
            order[3] = 0; order[4] = 1; order[5] = 2;
        }
        else if (diff < 0) {
            // left
            order[0] = 0; order[1] = 1; order[2] = 2;
            order[3] = 3; order[4] = 4; order[5] = 5;
        }
        int i = 0;
        while (nseg > 0) {
            seg[order[i]] = false;
            i += 1;
            nseg -= 1;
        }

        // left segments
        drawSegment(0, 54, 46, 24, 75, 24, 0, 90, commonData->fgcolor, seg[2]);
        drawSegment(0, 100, 82, 24, 112, 24, 50, 100, commonData->fgcolor, seg[1]);
        drawSegment(60, 100, 117, 24, 147, 24, 110, 100, commonData->fgcolor,seg[0]);
        // right segments
        drawSegment(340, 100, 283, 24, 253, 24, 290, 100, commonData->fgcolor, seg[3]);
        drawSegment(399, 100, 318, 24, 289, 24, 350, 100, commonData->fgcolor, seg[4]);
        drawSegment(399, 54, 354, 24, 325, 24, 399, 90, commonData->fgcolor, seg[5]);

        // Key Layout
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(commonData->keylock == false){
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                getdisplay().setCursor(343, 296);
                getdisplay().print("[ILUM]");
            }
        }

        // Update display
        getdisplay().nextPage();    // Partial update (fast)

    };
};

static Page* createPage(CommonData &common){
    return new PageXTETrack(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageXTETrack(
    "XTETrack",            // Page name
    createPage,             // Action
    0,                      // Number of bus values depends on selection in Web configuration
    {"XTE", "COG", "DTW", "BTW"}, // Bus values we need in the page
    true                    // Show display header on/off
);

#endif
