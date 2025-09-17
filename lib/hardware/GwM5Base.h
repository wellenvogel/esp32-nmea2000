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
//M5 Base Boards
#ifndef _GWM5BASE_H
#define _GWM5BASE_H
//M5 Serial (Atomic RS232 Base)
#ifdef M5_SERIAL_KIT_232 
  GWRESOURCE_USE(BASE,M5_SERIAL_KIT_232)
  GWRESOURCE_USE(SERIAL1,M5_SERIAL_KIT_232)
  #define _GWI_SERIAL1 BOARD_LEFT1,BOARD_LEFT2,GWSERIAL_TYPE_BI
#endif

//M5 Serial (Atomic RS485 Base)
#ifdef M5_SERIAL_KIT_485
  GWRESOURCE_USE(BASE,M5_SERIAL_KIT_485)
  GWRESOURCE_USE(SERIAL1,M5_SERIAL_KIT_485)
  #define _GWI_SERIAL1 BOARD_LEFT1,BOARD_LEFT2,GWSERIAL_TYPE_UNI
#endif
//M5 GPS (Atomic GPS Base)
#ifdef M5_GPS_KIT
  GWRESOURCE_USE(BASE,M5_GPS_KIT)
  GWRESOURCE_USE(SERIAL1,M5_GPS_KIT)
  #define _GWI_SERIAL1 BOARD_LEFT1,-1,GWSERIAL_TYPE_RX,9600
#endif
#ifdef M5_GPSV2_KIT
  GWRESOURCE_USE(BASE,M5_GPSV2_KIT)
  GWRESOURCE_USE(SERIAL1,M5_GPSV2_KIT)
  #define _GWI_SERIAL1 BOARD_LEFT1,-1,GWSERIAL_TYPE_RX,115200
#endif

//M5 ProtoHub
#ifdef M5_PROTO_HUB
  GWRESOURCE_USE(BASE,M5_PROTO_HUB)
  #define PPIN22 BOARD_LEFT1
  #define PPIN19 BOARD_LEFT2
  #define PPIN23 BOARD_LEFT3
  #define PPIN33 BOARD_LEFT4
  #define PPIN21 BOARD_RIGHT1
  #define PPIN25 BOARD_RIGHT2
#endif

//M5 PortABC extension
#ifdef M5_PORTABC
  GWRESOURCE_USE(BASE,M5_PORTABC)
  #define GROOVEA_PIN_2 BOARD_RIGHT2
  #define GROOVEA_PIN_1  BOARD_RIGHT1
  #define GROOVEB_PIN_2 BOARD_LEFT3
  #define GROOVEB_PIN_1  BOARD_LEFT4
  #define GROOVEC_PIN_2 BOARD_LEFT1
  #define GROOVEC_PIN_1  BOARD_LEFT2 
#endif

//can kit for M5 Atom
#if defined (M5_CAN_KIT) 
  GWRESOURCE_USE(BASE,M5_CAN_KIT)
  GWRESOURCE_USE(CAN,M5_CANKIT)
  #define ESP32_CAN_TX_PIN BOARD_LEFT1
  #define ESP32_CAN_RX_PIN BOARD_LEFT2
#endif

#endif
