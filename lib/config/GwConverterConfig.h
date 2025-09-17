/*
  (C) Andreas Vogel andreas@wellenvogel.de
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _GWCONVERTERCONFIG_H
#define _GWCONVERTERCONFIG_H

#include "GWConfig.h"
#include "N2kTypes.h"
#include <map>

//list of configs for the PGN 130306 wind references
static std::map<tN2kWindReference,String> windConfigs={
  {N2kWind_True_water,GwConfigDefinitions::windmtra},
  {N2kWind_Apparent,GwConfigDefinitions::windmawa},
  {N2kWind_True_boat,GwConfigDefinitions::windmgna},
  {N2kWind_Magnetic,GwConfigDefinitions::windmmgd},
  {N2kWind_True_North,GwConfigDefinitions::windmtng},
};

class GwConverterConfig{
    public:
    class WindMapping{
      public:
      using Wind0183Type=enum{
        AWA_AWS,
        TWA_TWS,
        TWD_TWS,
        GWA_GWS,
        GWD_GWS
      };
      tN2kWindReference n2kType;  
      Wind0183Type nmea0183Type;
      bool valid=false;
      WindMapping(){}
      WindMapping(const tN2kWindReference &n2k,const Wind0183Type &n183):
        n2kType(n2k),nmea0183Type(n183),valid(true){}
      WindMapping(const tN2kWindReference &n2k,const String &n183):
        n2kType(n2k){
          if (n183 == "twa_tws"){
            nmea0183Type=TWA_TWS;
            valid=true;
            return;
          }
          if (n183 == "awa_aws"){
            nmea0183Type=AWA_AWS;
            valid=true;
            return;
          }
          if (n183 == "twd_tws"){
            nmea0183Type=TWD_TWS;
            valid=true;
            return;
          }
        }  
    };
      int minXdrInterval=100;
      int starboardRudderInstance=0; 
      int portRudderInstance=-1; //ignore
      int min2KInterval=50;
      int rmcInterval=1000;
      int rmcCheckTime=4000;
      int winst312=256;
      bool unmappedXdr=false;
      unsigned long xdrTimeout=60000;
      std::vector<WindMapping> windMappings;
      void init(GwConfigHandler *config, GwLog*logger){
        minXdrInterval=config->getInt(GwConfigDefinitions::minXdrInterval,100);
        xdrTimeout=config->getInt(GwConfigDefinitions::timoSensor);
        starboardRudderInstance=config->getInt(GwConfigDefinitions::stbRudderI,0);
        portRudderInstance=config->getInt(GwConfigDefinitions::portRudderI,-1);
        min2KInterval=config->getInt(GwConfigDefinitions::min2KInterval,50);
        if (min2KInterval < 10)min2KInterval=10;
        rmcCheckTime=config->getInt(GwConfigDefinitions::checkRMCt,4000);
        if (rmcCheckTime < 1000) rmcCheckTime=1000;
        rmcInterval=config->getInt(GwConfigDefinitions::sendRMCi,1000);
        if (rmcInterval < 0) rmcInterval=0;
        if (rmcInterval > 0 && rmcInterval <100) rmcInterval=100;
        unmappedXdr=config->getBool(GwConfigDefinitions::unknownXdr);
        winst312=config->getInt(GwConfigDefinitions::winst312,256);
        for (auto && it:windConfigs){
          String cfg=config->getString(it.second);
          WindMapping mapping(it.first,cfg);
          if (mapping.valid){
            LOG_DEBUG(GwLog::ERROR,"add wind mapping n2k=%d,nmea0183=%01d(%s)",
              (int)(mapping.n2kType),(int)(mapping.nmea0183Type),cfg.c_str());
            windMappings.push_back(mapping);
          }
        }
      }
      const WindMapping findWindMapping(const tN2kWindReference &n2k) const{
        for (const auto & it:windMappings){
          if (it.n2kType == n2k) return it;
        }
        return WindMapping();
      }
      const WindMapping findWindMapping(const WindMapping::Wind0183Type &n183) const{
        for (const auto & it:windMappings){
          if (it.nmea0183Type == n183) return it;
        }
        return WindMapping();
      }


  };
#endif