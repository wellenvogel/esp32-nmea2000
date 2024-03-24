/*
  (C) Andreas Vogel andreas@wellenvogel.de
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
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

class GwConverterConfig{
    public:
      int minXdrInterval=100;
      int starboardRudderInstance=0; 
      int portRudderInstance=-1; //ignore
      int min2KInterval=50;
      int rmcInterval=1000;
      int rmcCheckTime=4000;
      void init(GwConfigHandler *config){
        minXdrInterval=config->getInt(GwConfigDefinitions::minXdrInterval,100);
        starboardRudderInstance=config->getInt(GwConfigDefinitions::stbRudderI,0);
        portRudderInstance=config->getInt(GwConfigDefinitions::portRudderI,-1);
        min2KInterval=config->getInt(GwConfigDefinitions::min2KInterval,50);
        if (min2KInterval < 10)min2KInterval=10;
        rmcCheckTime=config->getInt(GwConfigDefinitions::checkRMCt,4000);
        if (rmcCheckTime < 1000) rmcCheckTime=1000;
        rmcInterval=config->getInt(GwConfigDefinitions::sendRMCi,1000);
        if (rmcInterval < 0) rmcInterval=0;
        if (rmcInterval > 0 && rmcInterval <100) rmcInterval=100;
      }
  };
#endif