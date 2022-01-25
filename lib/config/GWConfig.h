#ifndef _GWCONFIG_H
#define _GWCONFIG_H
#include <Arduino.h>
#include <Preferences.h>
#include "GwLog.h"
#include "GwConfigItem.h"
#include "GwConfigDefinitions.h"
#include <map>


class GwConfigHandler: public GwConfigDefinitions{
    private:
        Preferences prefs;
        GwLog *logger;
        typedef std::map<String,String> StringMap;
        StringMap changedValues;
        boolean allowChanges=true;
    public:
        public:
        GwConfigHandler(GwLog *logger);
        bool loadConfig();
        bool saveConfig();
        void stopChanges();
        bool updateValue(String name, String value);
        bool reset(bool save);
        String toString() const;
        String toJson() const;
        String getString(const String name,const String defaultv="") const;
        bool getBool(const String name,bool defaultv=false) const ;
        int getInt(const String name,int defaultv=0) const;
        GwConfigInterface * getConfigItem(const String name, bool dummy=false) const;
        /**
         * change the value of a config item
         * will become a noop after stopChanges has been called 
         * !use with care! no checks of the value
         */
        bool setValue(String name, String value);
    private:
};
#endif