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

  //SERIAL_MODE can be: UNI (RX or TX only), BI (both), RX, TX
  //board specific pins
  #ifdef BOARD_M5ATOM
    #define ESP32_CAN_TX_PIN GPIO_NUM_22
    #define ESP32_CAN_RX_PIN GPIO_NUM_19
    //150mA if we power from the bus
    #define N2K_LOAD_LEVEL 3 
    #define GWBUTTON_PIN GPIO_NUM_39
    #define GWBUTTON_ACTIVE LOW
    //if GWBUTTON_PULLUPDOWN we enable a pulup/pulldown
    #define GWBUTTON_PULLUPDOWN
    //led handling
    //if we define GWLED_FASTNET the arduino fastnet lib is used
    #define GWLED_FASTLED
    #define GWLED_TYPE SK6812
    //color schema for fastled
    #define GWLED_SCHEMA GRB
    #define GWLED_PIN  GPIO_NUM_27
    //brightness 0...255
    #define GWLED_BRIGHTNESS 64
  #endif
  #ifdef BOARD_M5ATOM_CANUNIT
    #define ESP32_CAN_TX_PIN GPIO_NUM_26
    #define ESP32_CAN_RX_PIN GPIO_NUM_32
    //150mA if we power from the bus
    #define N2K_LOAD_LEVEL 3 
    //if using tail485
    #define GWSERIAL_TX 26
    #define GWSERIAL_RX 32
    #define GWSERIAL_MODE "UNI"
    #define GWBUTTON_PIN GPIO_NUM_39
    #define GWBUTTON_ACTIVE LOW
    //if GWBUTTON_PULLUPDOWN we enable a pulup/pulldown
    #define GWBUTTON_PULLUPDOWN 
    //led handling
    //if we define GWLED_FASTNET the arduino fastnet lib is used
    #define GWLED_FASTLED
    #define GWLED_TYPE SK6812
    //color schema for fastled
    #define GWLED_SCHEMA GRB
    #define GWLED_PIN  GPIO_NUM_27
    //brightness 0...255
    #define GWLED_BRIGHTNESS 64
  #endif
  #ifdef BOARD_M5STICK_CANUNIT
    #define ESP32_CAN_TX_PIN GPIO_NUM_32
    #define ESP32_CAN_RX_PIN GPIO_NUM_33
    //150mA if we power from the bus
    #define N2K_LOAD_LEVEL 3 
    //if using tail485
    #define GWSERIAL_TX 32
    #define GWSERIAL_RX 33
    #define GWSERIAL_MODE "UNI"
  #endif
  #ifdef BOARD_HOMBERGER
    #define ESP32_CAN_TX_PIN GPIO_NUM_5
    #define ESP32_CAN_RX_PIN GPIO_NUM_4
    //serial input only
    #define GWSERIAL_RX 16
    #define GWSERIAL_MODE "RX"
    #define GWBUTTON_PIN GPIO_NUM_0
    #define GWBUTTON_ACTIVE LOW
    //if GWBUTTON_PULLUPDOWN we enable a pulup/pulldown
    #define GWBUTTON_PULLUPDOWN 
  #endif

#endif
