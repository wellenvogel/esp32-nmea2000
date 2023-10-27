#ifndef GWIEXAMPLETASK_H
#define GWIEXAMPLETASK_H
#include "GwApi.h"
/**
 * an interface for the example task
*/
class ExampleTaskIf : public GwApi::TaskInterfaces::Base{
    public:
    long count=0;
    String someValue;
};
DECLARE_TASKIF(ExampleTaskIf);

#endif