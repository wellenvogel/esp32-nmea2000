/*
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

  unfortunately there is some typo here: M5 uses GROVE for their connections
  but we have GROOVE here.
  But to maintain compatibility to older build commands we keep the (wrong) wording
*/
#ifdef _NOGWHARDWAREUT
  #error "you are not allowed to include GwHardware.h in your user task header"
#endif
#ifndef _GWHARDWARE_H
#define _GWHARDWARE_H
#define GWSERIAL_TYPE_UNI 1
#define GWSERIAL_TYPE_BI 2
#define GWSERIAL_TYPE_RX 3
#define GWSERIAL_TYPE_TX 4
#include <GwConfigItem.h>
#include <HardwareSerial.h>
#include "GwAppInfo.h"
#include "GwUserTasks.h"

#ifdef GW_PINDEFS
  #define GWRESOURCE_USE(RES,USER) \
    __MSG(#RES " used by " #USER) \
    static int _resourceUsed ## RES =1;
#else
  #define GWRESOURCE_USE(...)
#endif

#ifndef CFG_INIT
  #define CFG_INIT(...)
#endif
//general definitions for M5AtomLite
//hint for groove pins:
//according to some schematics the numbering is 1,2,3(VCC),4(GND)
#ifdef PLATFORM_BOARD_M5STACK_ATOM
  #define GROOVE_PIN_2 GPIO_NUM_26
  #define GROOVE_PIN_1 GPIO_NUM_32
  #define GWBUTTON_PIN GPIO_NUM_39
  #define GWLED_FASTLED
  #define GWLED_TYPE SK6812
  //color schema for fastled
  #define GWLED_SCHEMA GRB
  #define GWLED_PIN  GPIO_NUM_27
  #define GWBUTTON_ACTIVE LOW
  //if GWBUTTON_PULLUPDOWN we enable a pulup/pulldown
  #define GWBUTTON_PULLUPDOWN
  #define BOARD_LEFT1 GPIO_NUM_22
  #define BOARD_LEFT2 GPIO_NUM_19
  #define BOARD_LEFT3 GPIO_NUM_23
  #define BOARD_LEFT4 GPIO_NUM_33
  #define BOARD_RIGHT1 GPIO_NUM_21
  #define BOARD_RIGHT2 GPIO_NUM_25
  #define USBSerial Serial
#endif

//general definitiones for M5AtomS3
#ifdef PLATFORM_BOARD_M5STACK_ATOMS3
  #define GROOVE_PIN_2 GPIO_NUM_2
  #define GROOVE_PIN_1 GPIO_NUM_1
  #define GWBUTTON_PIN GPIO_NUM_41
  #define GWLED_FASTLED
  #define GWLED_TYPE WS2812
  //color schema for fastled
  #define GWLED_SCHEMA GRB
  #define GWLED_PIN  GPIO_NUM_35
  #define GWBUTTON_ACTIVE LOW
  //if GWBUTTON_PULLUPDOWN we enable a pulup/pulldown
  #define GWBUTTON_PULLUPDOWN
  #define BOARD_LEFT1 GPIO_NUM_5
  #define BOARD_LEFT2 GPIO_NUM_6
  #define BOARD_LEFT3 GPIO_NUM_7
  #define BOARD_LEFT4 GPIO_NUM_8
  #define BOARD_RIGHT1 GPIO_NUM_39
  #define BOARD_RIGHT2 GPIO_NUM_38
#endif
//M5Stick C
#ifdef PLATFORM_BOARD_M5STICK_C
  #define GROOVE_PIN_2 GPIO_NUM_32
  #define GROOVE_PIN_1 GPIO_NUM_31
  #define USBSerial Serial
#endif

//NodeMCU 32 S
#ifdef PLATFORM_BOARD_NODEMCU_32S
  #define USBSerial Serial
#endif

#ifdef BOARD_M5ATOM
#define M5_CAN_KIT
//150mA if we power from the bus
#define N2K_LOAD_LEVEL 3 
//if using tail485
#define SERIAL_GROOVE_485 1
//brightness 0...255
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOMS3
#define M5_CAN_KIT 1
//150mA if we power from the bus
#define N2K_LOAD_LEVEL 3 
//if using tail485
#define SERIAL_GROOVE_485
//brightness 0...255
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOM_CANUNIT
#define M5_CANUNIT 1
#define GWLED_BRIGHTNESS 64
//150mA if we power from the bus
#define N2K_LOAD_LEVEL 3 
#endif

#ifdef BOARD_M5ATOMS3_CANUNIT
#define M5_CANUNIT 1
#define GWLED_BRIGHTNESS 64
#endif


#ifdef BOARD_M5ATOM_RS232_CANUNIT
#define M5_CANUNIT 1
#define M5_SERIAL_KIT_232
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOM_RS485_CANUNIT
#define M5_SERIAL_KIT_485
#define M5_CANUNIT 1
#define GWLED_BRIGHTNESS 64
#endif


#ifdef BOARD_M5STICK_CANUNIT
#define M5_CANUNIT 1
#endif

#ifdef BOARD_HOMBERGER
#define ESP32_CAN_TX_PIN GPIO_NUM_5
#define ESP32_CAN_RX_PIN GPIO_NUM_4
//serial input only
#define GWSERIAL_RX GPIO_NUM_16
#define GWSERIAL_TYPE GWSERIAL_TYPE_RX

#define GWBUTTON_PIN GPIO_NUM_0
#define GWBUTTON_ACTIVE LOW
//if GWBUTTON_PULLUPDOWN we enable a pulup/pulldown
#define GWBUTTON_PULLUPDOWN 
#endif

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
  #define _GWI_SERIAL1 BOARD_LEFT1,-1,GWSERIAL_TYPE_UNI,"9600"
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

//below we define the final device config based on the above
//boards and peripherals
//this allows us to easily also set them from outside
//serial adapter at the M5 groove pins
//we use serial2 for groove serial if serial1 is already defined
//before (e.g. by serial kit)
#ifdef SERIAL_GROOVE_485
  GWRESOURCE_USE(GROOVE,SERIAL_GROOVE_485)
  #define _GWI_SERIAL_GROOVE GWSERIAL_TYPE_UNI
#endif
#ifdef SERIAL_GROOVE_485_A
  GWRESOURCE_USE(GROOVEA,SERIAL_GROOVE_485_A)
  #define _GWI_SERIAL_GROOVE_A GWSERIAL_TYPE_UNI
#endif
#ifdef SERIAL_GROOVE_485_B
  GWRESOURCE_USE(GROOVEB,SERIAL_GROOVE_485_B)
  #define _GWI_SERIAL_GROOVE_B GWSERIAL_TYPE_UNI
#endif
#ifdef SERIAL_GROOVE_485_C
  GWRESOURCE_USE(GROOVEC,SERIAL_GROOVE_485_C)
  #define _GWI_SERIAL_GROOVE_C GWSERIAL_TYPE_UNI
#endif
#ifdef SERIAL_GROOVE_232
  GWRESOURCE_USE(GROOVE,SERIAL_GROOVE_232)
  #define _GWI_SERIAL_GROOVE GWSERIAL_TYPE_BI
#endif
#ifdef SERIAL_GROOVE_232_A
  GWRESOURCE_USE(GROOVEA,SERIAL_GROOVE_232_A)
  #define _GWI_SERIAL_GROOVE_A GWSERIAL_TYPE_BI
#endif
#ifdef SERIAL_GROOVE_232_B
  GWRESOURCE_USE(GROOVEB,SERIAL_GROOVE_232_B)
  #define _GWI_SERIAL_GROOVE_B GWSERIAL_TYPE_BI
#endif
#ifdef SERIAL_GROOVE_232_C
  GWRESOURCE_USE(GROOVEC,SERIAL_GROOVE_232_C)
  #define _GWI_SERIAL_GROOVE_C GWSERIAL_TYPE_BI
#endif




//http://docs.m5stack.com/en/unit/gps
#ifdef M5_GPS_UNIT
  GWRESOURCE_USE(GROOVE,M5_GPS_UNIT)
  #define _GWI_SERIAL_GROOVE GWSERIAL_TYPE_RX,"9600"
#endif
#ifdef M5_GPS_UNIT_A
  GWRESOURCE_USE(GROOVEA,M5_GPS_UNIT_A)
  #define _GWI_SERIAL_GROOVE_A GWSERIAL_TYPE_RX,"9600"
#endif
#ifdef M5_GPS_UNIT_B
  GWRESOURCE_USE(GROOVEB,M5_GPS_UNIT_B)
  #define _GWI_SERIAL_GROOVE_B GWSERIAL_TYPE_RX,"9600"
#endif
#ifdef M5_GPS_UNIT_C
  GWRESOURCE_USE(GROOVEC,M5_GPS_UNIT)
  #define _GWI_SERIAL_GROOVE_C GWSERIAL_TYPE_RX,"9600"
#endif




//can kit for M5 Atom
#ifdef M5_CAN_KIT
  GWRESOURCE_USE(BASE,M5_CAN_KIT)
  GWRESOURCE_USE(CAN,M5_CANKIT)
  #define ESP32_CAN_TX_PIN BOARD_LEFT1
  #define ESP32_CAN_RX_PIN BOARD_LEFT2
#endif
//CAN via groove 
#ifdef M5_CANUNIT
  GWRESOURCE_USE(GROOVE,M5_CANUNIT)
  GWRESOURCE_USE(CAN,M5_CANUNIT)
  #define ESP32_CAN_TX_PIN GROOVE_PIN_2
  #define ESP32_CAN_RX_PIN GROOVE_PIN_1
#endif

#ifdef M5_CANUNIT_A
  GWRESOURCE_USE(GROOVEA,M5_CANUNIT_A)
  GWRESOURCE_USE(CAN,M5_CANUNIT_A)
  #define ESP32_CAN_TX_PIN GROOVEA_PIN_2
  #define ESP32_CAN_RX_PIN GROOVEA_PIN_1
#endif
#ifdef M5_CANUNIT_B
  GWRESOURCE_USE(GROOVEB,M5_CANUNIT_B)
  GWRESOURCE_USE(CAN,M5_CANUNIT_B)
  #define ESP32_CAN_TX_PIN GROOVEB_PIN_2 
  #define ESP32_CAN_RX_PIN GROOVEA_PIN_1
#endif
#ifdef M5_CANUNIT_C
  GWRESOURCE_USE(GROOVEC,M5_CANUNIT_C)
  GWRESOURCE_USE(CAN,M5_CANUNIT_C)
  #define ESP32_CAN_TX_PIN GROOVEC_PIN_2
  #define ESP32_CAN_RX_PIN GROOVEC_PIN_1
#endif


#ifdef M5_ENV3
  #ifndef M5_GROOVEIIC
    #define M5_GROOVEIIC M5_ENV3
  #endif
  #ifndef GWSHT3X
    #define GWSHT3X -1
  #endif
  #ifndef GWQMP6988
    #define GWQMP6988 -1
  #endif
#endif

#ifdef M5_GROOVEIIC
  GROOVE_USE(M5_GROOVEIIC)
  #ifdef GWIIC_SCL
    #error "you cannot define both GWIIC_SCL and M5_GROOVEIIC"
  #endif
  #define GWIIC_SCL GROOVE_PIN_1
  #ifdef GWIIC_SDA
    #error "you cannot define both GWIIC_SDA and M5_GROOVEIIC"
  #endif
  #define GWIIC_SDA GROOVE_PIN_2
#endif

#ifdef _GWI_SERIAL_GROOVE
  #ifndef _GWI_SERIAL1
    #define _GWI_SERIAL1 GROOVE_PIN_1,GROOVE_PIN_2,_GWI_SERIAL_GROOVE
  #elif ! defined(_GWI_SERIAL2)
    #define _GWI_SERIAL2 GROOVE_PIN_1,GROOVE_PIN_2,_GWI_SERIAL_GROOVE
  #else
    #error "both serial devices already in use"
  #endif
#endif
#ifdef _GWI_SERIAL_GROOVE_A
  #ifndef _GWI_SERIAL1
    #define _GWI_SERIAL1 GROOVEA_PIN_1,GROOVEA_PIN_2,_GWI_SERIAL_GROOVE_A
  #elif ! defined(_GWI_SERIAL2)
    #define _GWI_SERIAL2 GROOVEA_PIN_1,GROOVEA_PIN_2,_GWI_SERIAL_GROOVE_A
  #else
    #error "both serial devices already in use"
  #endif
#endif
#ifdef _GWI_SERIAL_GROOVE_B
  #ifndef _GWI_SERIAL1
    #define _GWI_SERIAL1 GROOVEB_PIN_1,GROOVEB_PIN_2,_GWI_SERIAL_GROOVE_B
  #elif ! defined(_GWI_SERIAL2)
    #define _GWI_SERIAL2 GROOVEB_PIN_1,GROOVEB_PIN_2,_GWI_SERIAL_GROOVE_B
  #else
    #error "both serial devices already in use"
  #endif
#endif
#ifdef _GWI_SERIAL_GROOVE_C
  #ifndef _GWI_SERIAL1
    #define _GWI_SERIAL1 GROOVEC_PIN_1,GROOVEC_PIN_2,_GWI_SERIAL_GROOVE_C
  #elif ! defined(_GWI_SERIAL2)
    #define _GWI_SERIAL2 GROOVEC_PIN_1,GROOVEC_PIN_2,_GWI_SERIAL_GROOVE_C
  #else
    #error "both serial devices already in use"
  #endif  
#endif

#ifdef GWIIC_SDA
  #ifndef GWIIC_SCL
    #error "you must both define GWIIC_SDA and GWIIC_SCL"
  #endif
#endif
#ifdef GWIIC_SCL
  #ifndef GWIIC_SDA
    #error "you must both define GWIIC_SDA and GWIIC_SCL"
  #endif
  #define _GWIIC
#endif
#ifdef GWIIC_SDA2
  #ifndef GWIIC_SCL2
    #error "you must both define GWIIC_SDA2 and GWIIC_SCL2"
  #endif
#endif
#ifdef GWIIC_SCL2
  #ifndef GWIIC_SDA2
    #error "you must both define GWIIC_SDA and GWIIC_SCL2"
  #endif
  #define _GWIIC
#endif


#ifndef GWLED_TYPE
  #ifdef GWLED_CODE
  #if GWLED_CODE == 0
    #define GWLED_TYPE SK6812
  #endif
  #if GWLED_CODE == 1
    #define GWLED_TYPE WS2812
  #endif
  #endif
#endif
#ifdef GWLED_TYPE
  #define GWLED_FASTLED
  #ifndef GWLED_BRIGHTNESS
    #define GWLED_BRIGHTNESS 64
  #endif
#endif

#ifdef ESP32_CAN_TX_PIN
  #ifndef N2K_LOAD_LEVEL
    #define N2K_LOAD_LEVEL 3
  #endif
#endif

#ifdef GWLED_FASTLED
  #ifdef GWLED_BRIGHTNESS
    CFG_INIT(ledBrightness,GWSTRINGIFY(GWLED_BRIGHTNESS),NORMAL)
  #else
    CFG_INIT(ledBrightness,"64",NORMAL)
  #endif
#else
  CFG_INIT(ledBrightness,"64",HIDDEN)
#endif

#endif
