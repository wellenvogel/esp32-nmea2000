#ifndef _GWSHTXX_H
#define _GWSHTXX_H
#include "GwIicSensors.h"
#ifdef _GWIIC
    #if defined(GWSHT3X) || defined(GWSHT3X11) || defined(GWSHT3X12) || defined(GWSHT3X21) || defined(GWSHT3X22)
        #define _GWSHT3X
    #endif
    #if defined(GWSHT4X) || defined(GWSHT4X11) || defined(GWSHT4X12) || defined(GWSHT4X21) || defined(GWSHT4X22)
        #define _GWSHT4X
    #endif
#else
    #undef _GWSHT3X
    #undef GWSHT3X
    #undef GWSHT3X11
    #undef GWSHT3X12
    #undef GWSHT3X21
    #undef GWSHT3X22
    #undef _GWSHT4X
    #undef GWSHT4X
    #undef GWSHT4X11
    #undef GWSHT4X12
    #undef GWSHT4X21
    #undef GWSHT4X22
#endif
#ifdef _GWSHT3X
    #include "SHT3X.h"
#endif
#ifdef _GWSHT4X
    #include "SHT4X.h"
#endif
SensorBase::Creator registerSHT3X(GwApi *api);
SensorBase::Creator registerSHT4X(GwApi *api);
#endif