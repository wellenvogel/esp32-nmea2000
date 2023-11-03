#include "GwSHT3X.h"
#ifdef _GWSHT3X

bool SHT3X::init(uint8_t slave_addr_in, TwoWire* wire_in)
{
  _wire = wire_in;
  _address=slave_addr_in;
  return true;
}

byte SHT3X::get()
{
  unsigned int data[6];

  // Start I2C Transmission
  _wire->beginTransmission(_address);
  // Send measurement command
  _wire->write(0x2C);
  _wire->write(0x06);
  // Stop I2C transmission
  if (_wire->endTransmission()!=0) 
    return 1;  

  delay(200);

  // Request 6 bytes of data
  _wire->requestFrom(_address, (uint8_t) 6);

  // Read 6 bytes of data
  // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
  for (int i=0;i<6;i++) {
    data[i]=_wire->read();
  };
  
  delay(50);
  
  if (_wire->available()!=0) 
    return 2;

  // Convert the data
  cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
  fTemp = (cTemp * 1.8) + 32;
  humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

  return 0;
}
#endif