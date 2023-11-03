#ifndef _GQQMP6988_H
#define _GQQMP6988_H
#include "GwIicSensors.h"
#ifdef _GWIIC
    #if defined(GWQMP6988) || defined(GWQMP698811) || defined(GWQMP698812) || defined(GWQMP698821) || defined(GWQMP698822)
        #define _GWQMP6988
    #else
        #undef _GWQMP6988
    #endif
#else
    #undef _GWQMP6988
    #undef GWQMP6988
    #undef GWQMP698811
    #undef GWQMP698812
    #undef GWQMP698821
    #undef GWQMP698822
#endif
#ifdef _GWQMP6988
    #include "QMP6988.h"
#endif
void registerQMP6988(GwApi *api,SensorList &sensors);
#endif