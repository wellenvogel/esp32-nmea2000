#ifndef _GWSHT3X_H
#define _GWSHT3X_H
#include "GwIicSensors.h"
#ifdef _GWIIC
    #if defined(GWSHT3X) || defined(GWSHT3X11) || defined(GWSHT3X12) || defined(GWSHT3X21) || defined(GWSHT3X22)
        #define _GWSHT3X
    #endif
#else
    #undef _GWSHT3X
    #undef GWSHT3X
    #undef GWSHT3X11
    #undef GWSHT3X12
    #undef GWSHT3X21
    #undef GWSHT3X22
#endif
#ifdef _GWSHT3X
    #include "SHT3X.h"
#endif
IICSensorBase::Creator registerSHT3X(GwApi *api,IICSensorList &sensors);
#endif