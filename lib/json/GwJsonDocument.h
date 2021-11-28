#ifndef _GWJSONDOCUMENT_H
#define _GWJSONDOCUMENT_H
#include <ArduinoJson.h>

class GwJsonDocument : public DynamicJsonDocument{
    public:
        GwJsonDocument(int sz): DynamicJsonDocument(sz){}
};
#endif