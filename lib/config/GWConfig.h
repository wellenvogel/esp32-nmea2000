#ifndef _GWCONFIG_H
#define _GWCONFIG_H
#include <Arduino.h>
#include <Preferences.h>
#include "GwLog.h"
#include "GwConfigItem.h"
#include "GwConfigDefinitions.h"


class GwConfigHandler: public GwConfigDefinitions{
    private:
        Preferences prefs;
        GwLog *logger;
    public:
        public:
        GwConfigHandler(GwLog *logger);
        bool loadConfig();
        bool saveConfig();
        bool updateValue(const char *name, const char * value);
        bool updateValue(String name, String value);
        bool reset(bool save);
        String toString() const;
        String toJson() const;
        String getString(const String name) const;
        bool getBool(const String name) const ;
        int getInt(const String name) const;
        GwConfigItem * findConfig(const String name, bool dummy=false);
        GwConfigInterface * getConfigItem(const String name, bool dummy=false) const;
    private:
};
#endif