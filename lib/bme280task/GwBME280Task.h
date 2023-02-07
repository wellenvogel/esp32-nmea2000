#include  "GwApi.h"

//#if defined BOARD_SMOOTHFROGGY && defined HAS_BME280
#ifdef HAS_BME280

// I2C pins to use
#define BME_SDA 21
#define BME_SCL 22

void BME280Task(void *param);

//make the task known to the core with 4000 bits for the stack (default is 2000 and is too low)
DECLARE_USERTASK_PARAM(BME280Task,4000);

// Declare a capability that can be used in config.json to only
//   show some elements when this capability is set correctly
DECLARE_CAPABILITY(temperature,true); 
DECLARE_CAPABILITY(pressure,true);
DECLARE_CAPABILITY(humidity,true);

#endif