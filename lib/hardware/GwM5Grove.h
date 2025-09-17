/*
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

  unfortunately there is some typo here: M5 uses GROVE for their connections
  but we have GROOVE here.
  But to maintain compatibility to older build commands we keep the (wrong) wording
*/
//M5 Grove stuff
#ifndef _GW5MGROVE_H
#define _GW5MGROVE_H
#ifndef GROOVE_IIC
  #define GROOVE_IIC(...)
#endif
#include "GwM5GroveGen.h"

#if defined(_GWI_IIC1) || defined (_GWI_IIC2)
  #define _GWIIC
#endif


#endif