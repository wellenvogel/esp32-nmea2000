#ifndef _OBP60FUNCTIONS_H
#define _OBP60FUNCTIONS_H

#include <Arduino.h>
#include "OBP60Hardware.h"
  
// Global vars

// Routine for TTP229-BSF (!!! IC use not a I2C bus !!!)
// 
// 8 Key Mode
//
// Key number        {0, 1, 2, 3, 4, 5, 6, 7, 8}
// Scan code         {0, 8, 7, 1, 6, 2, 5, 3, 4}
int keyposition[9] = {0, 3, 5, 7, 8, 6, 4, 2, 1};   // Position of key in raw data, 0 = nothing touched
int keypad[9];          // Raw data array from TTP229
int key;                // Value of key [0|1], 0 = touched, 1 = not touched
int keycode = 0;        // Keycode of pressed key [0...8], 0 = nothing touched
String keystatus = "0"; // Status of key (0 = processed, 1s...8s, 1l...8l, left, right unprocessed)
int keycodeold;
int swipedelay = 500;   // Delay after swipe in [ms]
int keydelay = 500;     // Delay after key pressed in  [ms]
bool keylock = 0;       // Key lock after pressed key is valid (repeat protection by conginous pressing)
int repeatnum;          // Actual number of key detections for a pressed key
int shortpress = 5;     // number of key detections after that the key is valid (n * 40ms) for short pessing
int longpress = 25;     // number of key detections after that the key is valid (n * 40ms) for long pessing
int swipedir = 0;       // Swipe direction 0 = nothing, -1 = left, +1 = right


void readKeypad() {
  noInterrupts();
  pinMode(TTP_SDO, INPUT);
  pinMode(TTP_SCL, OUTPUT);
  keycode = 0;
  // Read key code from raw data
  for (int i = 0; i < 9; i++) {
      digitalWrite(TTP_SCL, LOW);
      delay(1);  // 2ms clock
      keypad[i] = digitalRead(TTP_SDO);
      if(i > 0){
          // Invert keypad
          if(keypad[i] == 1){
            key = 0;
          }
          else{
            key = 1;
          }
          keycode += key * i;
      }
      digitalWrite(TTP_SCL, HIGH);
      delay(1);
  }
  // Remapping keycode
  keycode = keyposition[keycode];

  // Read repeat number
  if(keycode == keycodeold){
    repeatnum ++;
  }
/*  
  // Detect swipe right
  if (keycode > 0 && keycodeold > 0 && keycode > keycodeold && keycode != keycodeold && keylock == false){
    keylock = true;
    swipedir = 1;
    keystatus = "right";
    buzzer(TONE3 buzPower, 100);
    delay(swipedelay);
  }
  // Detect swipe left
  if (keycode > 0 && keycodeold > 0 && keycode < keycodeold && keycode != keycodeold && keylock == false){
    keylock = true;
    swipedir = -1;
    keystatus = "left";
    buzzer(TONE3, buzPower, 100);
    delay(swipedelay);
  }
*/  
  // Detect short perssed keynumber
  if (keycode > 0 && repeatnum >= shortpress && repeatnum < longpress && keycode == keycodeold && keylock == false){
    keylock = true;
    keystatus = String(keycode) + "s";
    buzzer(TONE4, buzPower, 100);
    delay(keydelay);
  }
  /*
  // Detect long perssed keynumber
  if (keycode > 0 && repeatnum >= longpress && keylock == false){
    keystatus = String(keycode) + "l";
    buzzer(4000, 20, 200);
    keylock = true;
  }
  */
  // Reset keylock after release
  if (keycode == 0 && keycodeold == 0){
    keylock = false;
    repeatnum = 0;
  }
  // Copy keycode
  keycodeold = keycode;
  interrupts();
}

#endif