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
#define SERIAL_GROOVE_485
//brightness 0...255
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOMS3
#define M5_CAN_KIT
//150mA if we power from the bus
#define N2K_LOAD_LEVEL 3 
//if using tail485
#define SERIAL_GROOVE_485
//brightness 0...255
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOM_CANUNIT
#define M5_CANUNIT
#define GWLED_BRIGHTNESS 64
//150mA if we power from the bus
#define N2K_LOAD_LEVEL 3 
#endif

#ifdef BOARD_M5ATOMS3_CANUNIT
#define M5_CANUNIT
#define GWLED_BRIGHTNESS 64
#endif


#ifdef BOARD_M5ATOM_RS232_CANUNIT
#define M5_CANUNIT
#define M5_SERIAL_KIT_232
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOM_RS485_CANUNIT
#define M5_SERIAL_KIT_485
#define M5_CANUNIT
#define GWLED_BRIGHTNESS 64
#endif


#ifdef BOARD_M5STICK_CANUNIT
#define M5_CANUNIT
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
  #define _GWM5_BOARD
  #define GWSERIAL_TX BOARD_LEFT2
  #define GWSERIAL_RX BOARD_LEFT1
  #define GWSERIAL_TYPE GWSERIAL_TYPE_BI
#endif

//M5 Serial (Atomic RS485 Base)
#ifdef M5_SERIAL_KIT_485
  #ifdef _GWM5_BOARD
    #error "can only define one M5 base"
  #endif
  #define _GWM5_BOARD
  #define GWSERIAL_TX BOARD_LEFT2
  #define GWSERIAL_RX BOARD_LEFT1
  #define GWSERIAL_TYPE GWSERIAL_TYPE_UNI
#endif

//M5 GPS (Atomic GPS Base)
#ifdef M5_GPS_KIT
  #ifdef _GWM5_BOARD
    #error "can only define one M5 base"
  #endif
  #define _GWM5_BOARD
  #define GWSERIAL_RX BOARD_LEFT1
  #define GWSERIAL_TYPE GWSERIAL_TYPE_RX
  #define CFGDEFAULT_serialBaud "9600"
  #define CFGMODE_serialBaud GwConfigInterface::READONLY
#endif

//below we define the final device config based on the above
//boards and peripherals
//this allows us to easily also set them from outside
//serial adapter at the M5 groove pins
//we use serial2 for groove serial if serial1 is already defined
//before (e.g. by serial kit)
#ifdef SERIAL_GROOVE_485
  #define _GWM5_GROOVE
  #ifdef GWSERIAL_TYPE
    #define GWSERIAL2_TX GROOVE_PIN_2
    #define GWSERIAL2_RX GROOVE_PIN_1
    #define GWSERIAL2_TYPE GWSERIAL_TYPE_UNI
  #else
    #define GWSERIAL_TX GROOVE_PIN_2
    #define GWSERIAL_RX GROOVE_PIN_1
    #define GWSERIAL_TYPE GWSERIAL_TYPE_UNI
  #endif 
#endif
#ifdef SERIAL_GROOVE_232
  #ifdef _GWM5_GROOVE
    #error "can only have one groove device"
  #endif
  #define _GWM5_GROOVE
  #ifdef GWSERIAL_TYPE
    #define GWSERIAL2_TX GROOVE_PIN_2
    #define GWSERIAL2_RX GROOVE_PIN_1
    #define GWSERIAL2_TYPE GWSERIAL_TYPE_BI
  #else
    #define GWSERIAL_TX GROOVE_PIN_2
    #define GWSERIAL_RX GROOVE_PIN_1
    #define GWSERIAL_TYPE GWSERIAL_TYPE_BI
  #endif
#endif

//http://docs.m5stack.com/en/unit/gps
#ifdef M5_GPS_UNIT
  #ifdef _GWM5_GROOVE
    #error "can only have one M5 groove"
  #endif
  #define _GWM5_GROOVE
  #ifdef GWSERIAL_TYPE
    #define GWSERIAL2_RX GROOVE_PIN_1
    #define GWSERIAL2_TYPE GWSERIAL_TYPE_RX
    #define CFGDEFAULT_serialBaud "9600"
    #define CFGMODE_serialBaud GwConfigInterface::READONLY
  #else
    #define GWSERIAL_RX GROOVE_PIN_1
    #define GWSERIAL_TYPE GWSERIAL_TYPE_RX
    #define CFGDEFAULT_serial2Baud "9600"
    #define CFGMODE_serial2Baud GwConfigInterface::READONLY
  #endif 
#endif

//can kit for M5 Atom
#ifdef M5_CAN_KIT
  #ifdef _GWM5_BOARD
    #error "can only define one M5 base"
  #endif
  #define _GWM5_BOARD
  #define ESP32_CAN_TX_PIN BOARD_LEFT1
  #define ESP32_CAN_RX_PIN BOARD_LEFT2
#endif
//CAN via groove 
#ifdef M5_CANUNIT
  #ifdef _GWM5_GROOVE
    #error "can only have one M5 groove"
  #endif
  #define _GWM5_GROOVE
  #define ESP32_CAN_TX_PIN GROOVE_PIN_2
  #define ESP32_CAN_RX_PIN GROOVE_PIN_1
#endif

#ifdef M5_ENV3
  #ifndef M5_GROOVEIIC
    #define M5_GROOVEIIC
  #endif
  #ifndef GWSHT3X
    #define GWSHT3X -1
  #endif
#endif

#ifdef M5_GROOVEIIC
  #ifdef _GWM5_GROOVE
    #error "can only have one M5 groove"
  #endif
  #define _GWM5_GROOVE
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
  #define CFGMODE_ledBrightness GwConfigInterface::NORMAL
  #ifdef GWLED_BRIGHTNESS
    #define CFGDEFAULT_ledBrightness GWSTRINGIFY(GWLED_BRIGHTNESS)
  #endif
#else
  #define CFGMODE_ledBrightness GwConfigInterface::HIDDEN
#endif

#endif
