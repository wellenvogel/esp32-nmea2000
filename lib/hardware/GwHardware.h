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
#ifndef _GWHARDWARE_H
#define _GWHARDWARE_H
#include "GwUserTasks.h"

//general definitions for M5AtomLite
#ifdef PLATFORM_BOARD_M5STACK_ATOM
  #define GROOVE_PIN_1 GPIO_NUM_26
  #define GROOVE_PIN_2 GPIO_NUM_32
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
#endif
//general definitiones for M5AtomS3
#ifdef PLATFORM_BOARD_M5STACK_ATOMS3
  #define GROOVE_PIN_1 GPIO_NUM_2
  #define GROOVE_PIN_2 GPIO_NUM_1
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

//serial adapter at the M5 groove pins
#define SERIAL_GROOVE(SMODE) \
  static const gpio_num_t GWSERIAL_TX=GROOVE_PIN_1; \
  static const gpio_num_t GWSERIAL_RX=GROOVE_PIN_2; \
  static const char* GWSERIAL_MODE=SMODE;

//M5 Serial (Atomic RS232 Base)
#define SERIAL_KIT(SMODE) \
  static const gpio_num_t GWSERIAL_TX=BOARD_LEFT2; \
  static const gpio_num_t GWSERIAL_RX=BOARD_LEFT1; \
  static const char* GWSERIAL_MODE=SMODE;


#define CANKIT \
  static const gpio_num_t ESP32_CAN_TX_PIN=BOARD_LEFT1; \
  static const gpio_num_t ESP32_CAN_RX_PIN=BOARD_LEFT2;

#define CANUNIT \
  static const gpio_num_t ESP32_CAN_TX_PIN=GROOVE_PIN_1; \
  static const gpio_num_t ESP32_CAN_RX_PIN=GROOVE_PIN_2;
//SERIAL_MODE can be: UNI (RX or TX only), BI (both), RX, TX
//board specific pins
#ifdef BOARD_M5ATOM
CANKIT
//150mA if we power from the bus
#define N2K_LOAD_LEVEL 3 
//if using tail485
SERIAL_GROOVE("UNI")
//brightness 0...255
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOM_CANUNIT
CANUNIT
#define GWLED_BRIGHTNESS 64
//150mA if we power from the bus
#define N2K_LOAD_LEVEL 3 
#endif

#ifdef BOARD_M5ATOMS3_CANUNIT
CANUNIT
#define GWLED_BRIGHTNESS 64
#endif


#ifdef BOARD_M5ATOM_RS232_CANUNIT
CANUNIT
SERIALKIT("BI")
#define GWLED_BRIGHTNESS 64
#endif

#ifdef BOARD_M5ATOM_RS485_CANUNIT
SERIALKIT("UNI")
CANUNIT
#define GWLED_BRIGHTNESS 64
#endif


#ifdef BOARD_M5STICK_CANUNIT
#define ESP32_CAN_TX_PIN GPIO_NUM_32
#define ESP32_CAN_RX_PIN GPIO_NUM_33
#endif
#ifdef BOARD_HOMBERGER
#define ESP32_CAN_TX_PIN GPIO_NUM_5
#define ESP32_CAN_RX_PIN GPIO_NUM_4
//serial input only
#define GWSERIAL_RX GPIO_NUM_16
#define GWSERIAL_MODE "RX"

#define GWBUTTON_PIN GPIO_NUM_0
#define GWBUTTON_ACTIVE LOW
//if GWBUTTON_PULLUPDOWN we enable a pulup/pulldown
#define GWBUTTON_PULLUPDOWN 
#endif

#endif
