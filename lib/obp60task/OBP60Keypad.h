#ifndef _OBP60FUNCTIONS_H
#define _OBP60FUNCTIONS_H

#include <Arduino.h>
#include "OBP60Hardware.h"
  
// Global vars

// Touch keypad over ESP32 touch sensor inputs  

int keypad[9];          // Raw data array for keys
int key;                // Value of key [0|1], 0 = touched, 1 = not touched
int keycode = 0;        // Keycode of pressed key [0...8], 0 = nothing touched
int keycode2 = 0;       // Keycode of very short pressed key [0...8], 0 = nothing touched
int keycodeold = 0;     // Old keycode
int keycodeold2 = 0;    // Old keycode for short pressed key
int keystatus = 0;      // Status of key [0...11]
bool keyoff = false;    // Disable all keys
int keydelay = 250;     // Delay after key pressed in  [ms]
bool keylock = false;   // Key lock after pressed key is valid (repeat protection by conginous pressing)
long starttime = 0;     // Start time point for pressed key

void initKeys(CommonData &commonData) {
    // coordinates for virtual keyboard keys

    static uint16_t top = 281;
    static uint16_t width = 65;
    static uint16_t height = 18;

    commonData.keydata[0].x = 0;
    commonData.keydata[0].y = top;
    commonData.keydata[0].w = width + 1;
    commonData.keydata[0].h = height;

    commonData.keydata[1].x = commonData.keydata[0].x + commonData.keydata[0].w + 1;
    commonData.keydata[1].y = top;
    commonData.keydata[1].w = width;
    commonData.keydata[1].h = height;

    commonData.keydata[2].x = commonData.keydata[1].x + commonData.keydata[1].w + 1;
    commonData.keydata[2].y = top;
    commonData.keydata[2].w = width;
    commonData.keydata[2].h = height;

    commonData.keydata[3].x = commonData.keydata[2].x + commonData.keydata[2].w + 1;
    commonData.keydata[3].y = top;
    commonData.keydata[3].w = width;
    commonData.keydata[3].h = height;

    commonData.keydata[4].x = commonData.keydata[3].x + commonData.keydata[3].w + 1;
    commonData.keydata[4].y = top;
    commonData.keydata[4].w = width;
    commonData.keydata[4].h = height;

    commonData.keydata[5].x = commonData.keydata[4].x + commonData.keydata[4].w + 1;
    commonData.keydata[5].y = top;
    commonData.keydata[5].w = width;
    commonData.keydata[5].h = height;
}

  #ifdef HARDWARE_V21
  // Keypad functions for original OBP60 hardware
  int readKeypad(GwLog* logger, uint thSensitivity) {
    
    // Touch sensor values
    // 35000 - Not touched
    // 50000 - Light toched with fingertip
    // 70000 - Touched
    // 170000 - Strong touched
    uint32_t touchthreshold = (thSensitivity * -1200) + 170000; // thSensitivity 0...100%

    keystatus = 0;      // Status of key [0...11], 0 = processed, 1...8 = key 1..8, 9 = right swipe , 10 = left swipe, 11 keys disabled
    keycode = 0;

    // Read key code
    if(touchRead(14) > touchthreshold){ // Touch pad 1
      keypad[1] = 1;
    }
    else{
      keypad[1] = 0;
    }
    if(touchRead(13) > touchthreshold){ // Touch pad 2
      keypad[2] = 1;
    }
    else{
      keypad[2] = 0;
    }
    if(touchRead(12) > touchthreshold){ // Touch pad 3
      keypad[3] = 1;
    }
    else{
      keypad[3] = 0;
    }
    if(touchRead(11) > touchthreshold){ // Touch pad 4
      keypad[4] = 1;
    }
    else{
      keypad[4] = 0;
    }
    if(touchRead(10) > touchthreshold){ // Touch pad 5
      keypad[5] = 1;
    }
    else{
      keypad[5] = 0;
    }
    if(touchRead(9) > touchthreshold){  // Touch pad 6
      keypad[6] = 1;
    }
    else{
      keypad[6] = 0;
    }
    // Nothing touched
/*    if(keypad[1] == 0 && keypad[2] == 0 && keypad[3] == 0 &&  keypad[4] == 0 && keypad[5] == 0 && keypad[6] == 0){
      keypad[0] = 1;
    }
    else{
      keypad[0] = 0;
    }  */

    for (int i = 1; i <= 6; i++) {
        if(i > 0){
            // Convert keypad to keycode
            if(keypad[i] == 1){
              key = 1;
            }
            else{
              key = 0;
            }
            keycode += key * i;
        }
    }

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
        logger->logDebug(GwLog::LOG,"Very short 20ms key touch: %d", keycode);

        // Process only valid keys
        if(keycode == 1 || keycode == 4 || keycode == 5 || keycode == 6){
          keycode2 = keycode;
        }
        // Clear by invalid keys
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
        logger->logDebug(GwLog::LOG,"Short 200ms key touch: %d", keycode);
        keystatus = keycode;
        keycode = 0;
        keycodeold = 0;
        keycode2 = 0;
        keycodeold2 = 0;
        buzzer(TONE4, 100);
        keylock = false;
        delay(keydelay);
      }
    }

    // System page with key 5 and 4 in fast series
    if (keycode2 == 5 && keycodeold2 == 4) {
        logger->logDebug(GwLog::LOG,"Keycode for system page");
      keycode = 0;
      keycodeold = 0;
      keycode2 = 0;
      keycodeold2 = 0;
      keystatus = 12;
      buzzer(TONE4, 50);
      delay(30);
      buzzer(TONE4, 50);
      delay(30);
      buzzer(TONE4, 50);
    }

    // Key lock with key 1 and 6 or 6 and 1 in fast series
    if((keycode2 == 1 && keycodeold2 == 6) || (keycode2 == 6 && keycodeold2 == 1)) {
      keycode = 0;
      keycodeold = 0;
      keycode2 = 0;
      keycodeold2 = 0;
      buzzer(TONE4, 1000);
      keylock = false;
      delay(keydelay);
      keyoff = !keyoff;
      keystatus = 11;
    }

    // Detect swipe right
    if (keyoff == false && keycode > 0 && keycodeold > 0 && keycode > keycodeold && !((keycode == 1 && keycodeold == 6) || (keycode == 6 && keycodeold == 1))){
    //if (keycode > 0 && keycodeold > 0 && keycode > keycodeold){
      keycode = 0;
      keycodeold = 0;
      keycode2 = 0;
      keycodeold2 = 0;
      keystatus = 9;
      buzzer(TONE3, 150);
      buzzer(TONE4, 150);
    }

    // Detect swipe left
    if (keyoff == false && keycode > 0 && keycodeold > 0 && keycode < keycodeold && !((keycode == 1 && keycodeold == 6) || (keycode == 6 && keycodeold == 1))){
    //if (keycode > 0 && keycodeold > 0 && keycode < keycodeold){  
      keycode = 0;
      keycodeold = 0;
      keycode2 = 0;
      keycodeold2 = 0;
      keystatus = 10;
      buzzer(TONE4, 150);
      buzzer(TONE3, 150);
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

  #ifdef HARDWARE_LIGHT
  int readSensorpads(){
      // Read key code 
      if(digitalRead(UP) == LOW){
        keycode = 10; // Left swipe
      }
      else if(digitalRead(DOWN) == LOW){
        keycode = 9;  // Right swipe
      }
      else if(digitalRead(CONF) == LOW){
        keycode = 3;  // Key 3
      }
      else if(digitalRead(MENUE) == LOW){
        keycode = 1;  // Key 1
      }
      else if(digitalRead(EXIT) == LOW){
        keycode = 2;  // Key 2
      }
      else{
        keycode = 0;  // No key activ
      }
      return keycode;
    }

  // Keypad functions for OBP60 clone (thSensitivity is inactiv)
  int readKeypad(GwLog* logger, uint thSensitivity) {
    pinMode(UP, INPUT);
    pinMode(DOWN, INPUT);
    pinMode(CONF, INPUT);
    pinMode(MENUE, INPUT);
    pinMode(EXIT, INPUT);

    // Raed pad values
    readSensorpads();

    // Detect key
    if (keycode > 0 ){
      if(keycode != keycodeold){
        starttime = millis();   // Start key pressed
        keycodeold = keycode;
      }
      // If key pressed longer than 200ms
      if(millis() > starttime + 200 && keycode == keycodeold) {
        keystatus = keycode;
        // Copy keycode
        keycodeold = keycode;
        while(readSensorpads() > 0){} // Wait for pad lesease
        delay(keydelay);
      }
    }
    else{
      keycode = 0;
      keycodeold = 0;
      keystatus = 0;
    }

    return keystatus;
  }
  #endif

#endif
