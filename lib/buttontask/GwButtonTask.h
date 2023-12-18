#ifndef _GWBUTTONTASK_H
#define _GWBUTTONTASK_H
#include "GwApi.h"
//task function
void initButtons(GwApi *param);
DECLARE_INITFUNCTION(initButtons);

class IButtonTask : public GwApi::TaskInterfaces::Base
{
public:
    typedef enum
    {
        OFF,
        PRESSED,
        PRESSED_5, // 5...10s
        PRESSED_10 //>10s
    } ButtonState;
    ButtonState state=OFF;
    long pressCount=0;
};
DECLARE_TASKIF(IButtonTask);
#endif