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
        const char * getCString(const String name, const char *defaultv="") const;
        GwConfigInterface * getConfigItem(const String name, bool dummy=false) const;
        bool checkPass(String hash);
        std::vector<String> getSpecial() const;
        int numSpecial() const;
        /**
         * change the value of a config item
         * will become a noop after stopChanges has been called 
         * !use with care! no checks of the value
         */
        bool setValue(String name, String value, bool hide=false);
        bool setValue(String name, String value, GwConfigInterface::ConfigType type);
        static void toHex(unsigned long v,char *buffer,size_t bsize);
        unsigned long getSaltBase(){return saltBase;}
        ~GwConfigHandler();
        bool userChangesAllowed(){return allowChanges;}
        template <typename T>
        bool getValue(T & target, const String &name, int defaultv=0){
            GwConfigInterface *i=getConfigItem(name);
            if (!i){
                target=(T)defaultv;
                return false;
            }
            target=(T)(i->asInt());
            return true;
        }
        bool getValue(int &target, const String &name, int defaultv=0){
            GwConfigInterface *i=getConfigItem(name);
            if (!i){
                target=defaultv;
                return false;
            }
            target=i->asInt();
            return true;
        }
        bool getValue(long &target, const String &name, long defaultv=0){
            GwConfigInterface *i=getConfigItem(name);
            if (!i){
                target=defaultv;
                return false;
            }
            target=i->asInt();
            return true;
        }
        bool getValue(float &target, const String &name, float defaultv=0){
            GwConfigInterface *i=getConfigItem(name);
            if (!i){
                target=defaultv;
                return false;
            }
            target=i->asFloat();
            return true;
        }
        bool getValue(bool &target, const String name, bool defaultv=false){
            GwConfigInterface *i=getConfigItem(name);
            if (!i){
                target=defaultv;
                return false;
            }
            target=i->asBoolean();
            return true;
        }
        bool getValue(String &target, const String name, const String &defaultv=""){
            GwConfigInterface *i=getConfigItem(name);
            if (!i){
                target=defaultv;
                return false;
            }
            target=i->asString();
            return true;
        }
    private:
        unsigned long saltBase=0;
        void populateConfigs(GwConfigInterface **);
};
#endif