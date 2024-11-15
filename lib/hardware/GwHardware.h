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
#define GWSERIAL_TYPE_UNK 0
#include <GwConfigItem.h>
#include <HardwareSerial.h>
#include "GwAppInfo.h"
#include "GwUserTasks.h"

#ifdef GW_PINDEFS
  #define GWRESOURCE_USE(RES,USER) \
    __MSG(#RES " used by " #USER) \
    static int _resourceUsed ## RES =1;
  #define __USAGE __MSG
#else
  #define GWRESOURCE_USE(...)
  #define __USAGE(...)
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

#include "GwM5Base.h"
#include "GwM5Grove.h"


#ifdef GWIIC_SDA
  #ifdef _GWI_IIC1
    #error "you must not define IIC1 on grove and GWIIC_SDA"
  #endif
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
  #ifdef _GWI_IIC2
    #error "you must not define IIC2 on grove and GWIIC_SDA2"
  #endif
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
