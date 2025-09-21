#! /usr/bin/env python3
#generate 3 byte codes for the RGB bytes
#refer to https://controllerstech.com/ws2812-leds-using-spi/
ONE_BIT='110'
ZERO_BIT='100'

currentStr=''

def checkAndPrint(curr):
    if len(curr) >= 8:
        print("0b%s,"%curr[0:8],end='')
        return curr[8:]
    return curr
first=True

print("uint8_t colorTo3Byte[256][3]=")
print("{")
for i in range(0,256):
    if not first:
        print("},")
    first=False
    print("{/*%02d*/"%i,end='')
    mask=0x80
    for b in range(0,8):
        if (i & mask) != 0:
            currentStr+=ONE_BIT
        else:
            currentStr+=ZERO_BIT
        mask=mask >> 1
        currentStr=checkAndPrint(currentStr)
print("}")
print("};")    
