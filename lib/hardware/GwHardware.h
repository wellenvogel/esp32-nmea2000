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
#ifndef CFG_INIT
  #define CFG_INIT(...)
#endif
#define CFG_INITP(prefix,suffix,value,mode) CFG_INIT(prefix ## suffix,value,mode)
#ifndef CFG_SERIAL
  #define CFG_SERIAL(...)
#endif

#ifdef GW_PINDEFS
  #define GWRESOURCE_USE(RES,USER) \
    __MSG(__STR(RES) __STR(USER) " used by " #USER) \
    static int __EXPAND3(_resourceUsed,RES,USER) =1;
  #define GWBASE_USE(USER) \
    __MSG("base unit " #USER) \
    static int _resourceUsedBase=1;  
  #define GW_SETGROOVE(prfx,p1,p2) \
    static int __EXPAND3(GROOVEPIN_,prfx,1) = p1; \
    static int __EXPAND3(GROOVEPIN_,prfx,2) = p2;
  #define GW_GETGROOVE(prfx,pin) \
    __EXPAND3(GROOVEPIN_,prfx,pin)
  #define GW_SETRES(res,groove,val) \
    static int __EXPAND3(_resource,res,groove) = val;
  #define GW_GETRES(res,groove) \
    __EXPAND3(_resource,res,groove)
#else
  #define GW_SETGROOVE(...)
  #define GW_GETGROOVE(...)
  #define GW_SETRES(...) 
  #define GW_GETRES(...) 
  #define GWRESOURCE_USE(...)
  #define GWBASE_USE(...)
#endif    
//general definitions for M5AtomLite
//hint for groove pins:
//according to some schematics the numbering is 1,2,3(VCC),4(GND)
#ifdef PLATFORM_BOARD_M5STACK_ATOM
  GW_SETGROOVE(1,GPIO_NUM_32,GPIO_NUM_26)
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
  GW_SETGROOVE(1,GPIO_NUM_1,GPIO_NUM_2)
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
  GW_SETGROOVE(1,GPIO_NUM_31,GPIO_NUM_32)
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
  GWBASE_USE(M5_SERIAL_KIT_232)
  CFG_SERIAL(BASE,1,BOARD_LEFT1,BOARD_LEFT2,GWSERIAL_TYPE_BI)
  #define _GW_BASE_SERIAL
#endif

//M5 Serial (Atomic RS485 Base)
#ifdef M5_SERIAL_KIT_485
  GWBASE_USE(M5_SERIAL_KIT_485)
  CFG_SERIAL(BASE,1,BOARD_LEFT1,BOARD_LEFT2,GWSERIAL_TYPE_UNI)
  #define _GW_BASE_SERIAL
#endif
//M5 GPS (Atomic GPS Base)#define GW_GETGROOVE(prfx,pin
#ifdef M5_GPS_KIT
  GWBASE_USE(M5_GPS_KIT)
  CFG_SERIAL(BASE,1,BOARD_LEFT1,-1,GWSERIAL_TYPE_UNI)
  CFG_INIT(serialBaud,"9600",READONLY)
  #define _GW_BASE_SERIAL
#endif

//M5 ProtoHub
#ifdef M5_PROTO_HUB
  GWBASE_USE(M5_PROTO_HUB)
  #define PPIN22 BOARD_LEFT1
  #define PPIN19 BOARD_LEFT2
  #define PPIN23 BOARD_LEFT3
  #define PPIN33 BOARD_LEFT4
  #define PPIN21 BOARD_RIGHT1
  #define PPIN25 BOARD_RIGHT2
#endif

//M5 PortABC extension
#ifdef M5_PORTABC
  GWBASE_USE(M5_PORTABC)
  //yellow pings are grove2
  GW_SETGROOVE(2,BOARD_RIGHT1,BOARD_RIGHT2)
  GW_SETGROOVE(3,BOARD_LEFT4,BOARD_LEFT3)
  GW_SETGROOVE(4,BOARD_LEFT2,BOARD_LEFT1)
#endif

#ifndef GW_GROOVE_SERIAL
  //GW_GROOVE_SERIAL can be 1,2
  #ifdef _GW_BASE_SERIAL
    #define GW_GROOVE_SERIAL 2
  #else
    #define GW_GROOVE_SERIAL 1
  #endif
#endif
#ifdef GW_GROOVE_SERIAL
  GW_SETRES(serial,1,GW_GROOVE_SERIAL)
#endif
#ifdef GW_GROOVE2_SERIAL
  GW_SETRES(serial,2,GW_GROOVE1_SERIAL)
#endif
#ifdef GW_GROOVE3_SERIAL
  GW_SETRES(serial,3,GW_GROOVE2_SERIAL)
#endif
#ifdef GW_GROOVE4_SERIAL
  GW_SETRES(serial,4,GW_GROOVE3_SERIAL)
#endif

static constexpr const char * serial2cfg(const int serial){
  CASSERT(serial==1 || serial == 2,"invalid serial");
  return (serial==1)?"serial":(serial==2)?"serial2":"";
}


//below we define the final device config based on the above
//boards and peripherals
//this allows us to easily also set them from outside
//serial adapter at the M5 groove pins
//we use serial2 for groove serial if serial1 is already defined
//before (e.g. by serial kit)
#ifdef SERIAL_GROOVE_485
  GWRESOURCE_USE(GROVE,SERIAL_GROOVE_485)
  CFG_SERIAL(SERIAL_GROOVE_485,GW_GETRES(serial,SERIAL_GROOVE_485),GW_GETGROOVE(SERIAL_GROOVE_485,1),GW_GETGROOVE(SERIAL_GROOVE_485,2),GWSERIAL_TYPE_UNI)
#endif
#ifdef SERIAL_GROOVE_232
  GWRESOURCE_USE(GROVE,SERIAL_GROOVE_232)
  CFG_SERIAL(SERIAL_GROOVE_232,GW_GETRES(serial,SERIAL_GROOVE_232),GW_GETGROOVE(SERIAL_GROOVE_232,1),GW_GETGROOVE(SERIAL_GROOVE_232,2),GWSERIAL_TYPE_BI)
#endif

//http://docs.m5stack.com/en/unit/gps
#ifdef M5_GPS_UNIT
  GWRESOURCE_USE(GROVE,M5_GPS_UNIT)
  CFG_SERIAL(M5_GPS_UNIT,GW_GETRES(serial,M5_GPS_UNIT),GW_GETGROOVE(M5_GPS_UNIT,1),-1,GWSERIAL_TYPE_RX)
  CFG_INITP(GW_GETRES(serial,M5_GPS_UNIT),Baud,"9600",READONLY)
#endif

//can kit for M5 Atom
#ifdef M5_CAN_KIT
  GWBASE_USE(M5_CAN_KIT)
  #define ESP32_CAN_TX_PIN BOARD_LEFT1
  #define ESP32_CAN_RX_PIN BOARD_LEFT2
#endif
//CAN via groove 
#ifdef M5_CANUNIT
  GWRESOURCE_USE(GROVE,M5_CANUNIT)
  #define ESP32_CAN_TX_PIN GW_GETGROOVE(M5_CANUNIT,2)
  #define ESP32_CAN_RX_PIN GW_GETGROOVE(M5_CANUNIT,1)
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
  GROVE_USE(M5_GROOVEIIC)
  #ifdef GWIIC_SCL
    #error "you cannot define both GWIIC_SCL and M5_GROOVEIIC"
  #endif
  #define GWIIC_SCL GROOVE_PIN_1
  #ifdef GWIIC_SDA
    #error "you cannot define both GWIIC_SDA and M5_GROOVEIIC"
  #endif
  #define GWIIC_SDA GROOVE_PIN_2
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
