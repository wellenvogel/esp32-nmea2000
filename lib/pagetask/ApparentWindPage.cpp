#include "Pagedata.h"

class ApparentWindPage : public Page
{
    int dummy=0; //an example on how you would maintain some status
                 //for a page
public:
    ApparentWindPage(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"created ApparentWindPage");
        dummy=1;
    }
    virtual void display(CommonData &commonData, PageData &pageData)
    {
        GwLog *logger = commonData.logger;
        dummy++;
        for (int i = 0; i < 2; i++)
        {
            GwApi::BoatValue *value = pageData.values[i];
            if (value == NULL)
                continue;
            LOG_DEBUG(GwLog::LOG, "drawing at apparentWindPage(%d),dummy=%d, p=%s,v=%f",
                      i,
                      dummy,
                      value->getName().c_str(),
                      value->valid ? value->value : -1.0);
        }
    };
};

static Page *createPage(CommonData &common){
    return new ApparentWindPage(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerApparentWindPage(
    "apparentWind",
    createPage,
    0,
    {GwBoatData::_AWA,GwBoatData::_AWS},
    false
);