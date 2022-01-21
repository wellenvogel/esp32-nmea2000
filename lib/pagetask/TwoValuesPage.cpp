#include "Pagedata.h"

class TwoValuesPage : public Page
{
public:
    TwoValuesPage(CommonData &comon){
        comon.logger->logDebug(GwLog::LOG,"created TwoValuePage");
        //add some initialization code here
    }
    virtual void display(CommonData &commonData, PageData &pageData)
    {
        GwLog *logger = commonData.logger;
        for (int i = 0; i < 2; i++)
        {
            GwApi::BoatValue *value = pageData.values[i];
            if (value == NULL)
                continue;
            LOG_DEBUG(GwLog::LOG, "drawing at twoValuesPage(%d), p=%s,v=%f",
                      i,
                      value->getName().c_str(),
                      value->valid ? value->value : -1.0);
        }
    };
};

static Page *createPage(CommonData &common){
    return new TwoValuesPage(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerTwoValuesPage(
    "twoValues",
    createPage,
    2
);