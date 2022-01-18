#include "Pagedata.h"

void oneValuePage(CommonData &commonData, PageData &pageData){
    GwLog *logger=commonData.logger;
    GwApi::BoatValue *value=pageData.values[0];
    if (value == NULL) return;
    LOG_DEBUG(GwLog::LOG,"drawing at oneValuePage, p=%s,v=%f",
        value->getName().c_str(),
        value->valid?value->value:-1.0
    );
};

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerOneValuePage(
    "oneValue",
    oneValuePage,
    1
);