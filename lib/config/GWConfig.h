#ifndef _GWCONFIG_H
#define _GWCONFIG_H
#include <Arduino.h>
#include "GwLog.h"
#include "GwConfigItem.h"
#include "GwConfigDefinitions.h"
#include <map>
#include <vector>

class Preferences;
class GwConfigHandler: public GwConfigDefinitions{
    private:
        Preferences *prefs;
        GwLog *logger;
        typedef std::map<String,String> StringMap;
        boolean allowChanges=true;
        GwConfigInterface **configs;
    public:
        public:
        GwConfigHandler(GwLog *logger);
        bool loadConfig();
        void stopChanges();
        bool updateValue(String name, String value);
        bool reset();
        String toString() const;
        String toJson() const;
        String getString(const String name,const String defaultv="") const;
        bool getBool(const String name,bool defaultv=false) const ;
        int getInt(const String name,int defaultv=0) const;
        GwConfigInterface * getConfigItem(const String name, bool dummy=false) const;
        bool checkPass(String hash);
        std::vector<String> getSpecial() const;
        int numSpecial() const;
        /**
         * change the value of a config item
         * will become a noop after stopChanges has been called 
         * !use with care! no checks of the value
         */
        bool setValue(String name, String value);
        static void toHex(unsigned long v,char *buffer,size_t bsize);
        unsigned long getSaltBase(){return saltBase;}
        ~GwConfigHandler();
        bool userChangesAllowed(){return allowChanges;}
    private:
        unsigned long saltBase=0;
        void populateConfigs(GwConfigInterface **);
};
#endif