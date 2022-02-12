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
int keycode2 = 0;       // Keycode of very short pressed key [0...8], 0 = nothing touched
int keycodeold = 0;     // Old keycode
int keycodeold2 = 0;    // Old keycode for short pressed key
bool keyoff = false;    // Disable all keys
int keydelay = 250;     // Delay after key pressed in  [ms]
bool keylock = false;   // Key lock after pressed key is valid (repeat protection by conginous pressing)
long starttime = 0;     // Start time point for pressed key


int readKeypad() {
  int keystatus = 0;      // Status of key [0...11], 0 = processed, 1...8 = key 1..8, 9 = right swipe , 10 = left swipe, 11 keys disabled

  noInterrupts();
  pinMode(TTP_SDO, INPUT);
  pinMode(TTP_SCL, OUTPUT);
  keycode = 0;
  // Read key code from raw data
  for (int i = 0; i < 9; i++) {
      digitalWrite(TTP_SCL, LOW);
      delay(0);  // 0ms clock
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
      delay(0);   // 0ms clock
  }
  interrupts();
  // Remapping keycode
  keycode = keyposition[keycode];

  // Detect short keynumber
  if (keycode > 0 ){ 
    if(keylock == false){
      starttime = millis();
      keylock = true;
    }
    if (keycode != keycodeold){
      keylock = false;
    }
    // Detect a very short keynumber (10ms)
    if (millis() > starttime + 10 && keycode == keycodeold && keylock == true) {
      // Process only valid keys
      if(keycode == 1 || keycode == 6){
        keycode2 = keycode;
      }
      // Clear by unvalid keys
      else{
        keycode2 = 0;
        keycodeold2 = 0;
      }
    }
    // Timeout for very short pressed key
    if(millis() > starttime + 200){
      keycode2 = 0;
    }
    // Detect a short keynumber (200ms)
    if (keyoff == false && millis() > starttime + 200 && keycode == keycodeold && keylock == true) {
      keystatus = keycode;
      keycode = 0;
      keycodeold = 0;
      keycode2 = 0;
      keycodeold2 = 0;
      buzzer(TONE4, buzPower, 100);
      keylock = false;
      delay(keydelay);
    }
  }

  // Key lock with key 1 and 6 or 6 and 1 in fast series
  if((keycode2 == 1 && keycodeold2 == 6) || (keycode2 == 6 && keycodeold2 == 1)) {
    keycode = 0;
    keycodeold = 0;
    keycode2 = 0;
    keycodeold2 = 0;
    buzzer(TONE4, buzPower, 1000);
    keylock = false;
    delay(keydelay);

    keyoff = !keyoff;
    if(keyoff == true){
      keystatus = 11;
    }
    else{
      keystatus = 0;
    }
  }

  // Detect swipe right
  if (keyoff == false && keycode > 0 && keycodeold > 0 && keycode > keycodeold && !((keycode == 1 && keycodeold == 6) || (keycode == 6 && keycodeold == 1))){
  //if (keycode > 0 && keycodeold > 0 && keycode > keycodeold){
    keycode = 0;
    keycodeold = 0;
    keycode2 = 0;
    keycodeold2 = 0;
    keystatus = 9;
    buzzer(TONE3, buzPower, 150);
    buzzer(TONE4, buzPower, 150);
  }

  // Detect swipe left
  if (keyoff == false && keycode > 0 && keycodeold > 0 && keycode < keycodeold && !((keycode == 1 && keycodeold == 6) || (keycode == 6 && keycodeold == 1))){
  //if (keycode > 0 && keycodeold > 0 && keycode < keycodeold){  
    keycode = 0;
    keycodeold = 0;
    keycode2 = 0;
    keycodeold2 = 0;
    keystatus = 10;
    buzzer(TONE4, buzPower, 150);
    buzzer(TONE3, buzPower, 150);
  }

  // Reset keylock after release
  if (keycode == 0){
    keylock = false;
  }

  // Copy keycode
  keycodeold = keycode;
  keycodeold2 = keycode2;

  return keystatus;
}

#endif