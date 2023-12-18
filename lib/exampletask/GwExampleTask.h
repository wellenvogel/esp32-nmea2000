#pragma once
#include "GwApi.h"
//we only compile for some boards
#ifdef BOARD_TEST
//we could add the following defines also in our local platformio.ini
//CAN base 
#define M5_CAN_KIT
//RS485 on groove
#define SERIAL_GROOVE_485

void exampleInit(GwApi *param);

//let the core call an init function before the 
//N2K Stuff and the communication is set up
//especially this init function will register the real task at the core
//this gives you some flexibility to decide based on config or defines whether you
//really want to start the task or not
//this function must return when done - otherwise the core will not start up
DECLARE_INITFUNCTION(exampleInit);

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