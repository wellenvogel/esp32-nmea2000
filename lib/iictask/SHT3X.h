#ifndef __SHT3X_H
#define __HT3X_H
//taken from https://github.com/m5stack/M5Unit-ENV/tree/0.0.8/src

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include "Wire.h"

class SHT3X{
public:
  bool init(uint8_t slave_addr_in=0x44, TwoWire* wire_in = &Wire);
  byte get(void);
  float cTemp=0;
  float fTemp=0;
  float humidity=0;

private:
  uint8_t _address;
  TwoWire* _wire;
};

#endif
