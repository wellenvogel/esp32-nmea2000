/******************************************************************************
 *
 * imglib.h
 *
 * SPDX-License-Identifier: MIT
 *****************************************************************************/
 
#ifndef _IMGLIB_H_
#define _IMGLIB_H_ 1

// extract bytes from an unsigned word
#define LOBYTE(x) ((x)&0xff)
#define HIBYTE(x) (((x) >> 8) & 0xff)

// GIF  encoding constants
#define DESCRIPTOR_INTRODUCER 0x2c
#define TERMINATOR_INTRODUCER 0x3b

#define LZ_MAX_CODE 4095 // Biggest code possible in 12 bits
#define LZ_BITS 12

#define FLUSH_OUTPUT 4096 // Impossible code, to signal flush
#define FIRST_CODE 4097   // Impossible code, to signal first
#define NO_SUCH_CODE 4098 // Impossible code, to signal empty

// magfic constants and declarations for GIF LZW

#define HT_SIZE 8192       /* 12bits = 4096 or twice as big! */
#define HT_KEY_MASK 0x1FFF /* 13bits keys */
#define HT_KEY_NUM_BITS 13 /* 13bits keys */
#define HT_MAX_KEY 8191    /* 13bits - 1, maximal code possible */
#define HT_MAX_CODE 4095   /* Biggest code possible in 12 bits. */

/* The 32 bits of the long are divided into two parts for the key & code:   */
/* 1. The code is 12 bits as our compression algorithm is limited to 12bits */
/* 2. The key is 12 bits Prefix code + 8 bit new char or 20 bits.       */
/* The key is the upper 20 bits.  The code is the lower 12. */
#define HT_GET_KEY(l) (l >> 12)
#define HT_GET_CODE(l) (l & 0x0FFF)
#define HT_PUT_KEY(l) (l << 12)
#define HT_PUT_CODE(l) (l & 0x0FFF)

typedef unsigned char GifPixelType;
typedef unsigned char GifByteType;

typedef struct GifHashTableType {
    uint32_t HTable[HT_SIZE];
} GifHashTableType;

typedef struct GifFilePrivateType {
    uint8_t BitsPerPixel;           // Bits per pixel (Codes uses at least this + 1)
    uint16_t ClearCode;             // The CLEAR LZ code
    uint16_t EOFCode;               // The EOF LZ code
    uint16_t RunningCode;           // The next code algorithm can generate
    uint16_t RunningBits;           // The number of bits required to represent RunningCode
    uint16_t MaxCode1;              // 1 bigger than max. possible code, in RunningBits bits
    uint16_t LastCode;              // The code before the current code.
    uint16_t CrntCode;              // Current algorithm code
    uint16_t CrntShiftState;        // Number of bits in CrntShiftDWord
    uint32_t CrntShiftDWord;        // For bytes decomposition into codes
    uint32_t PixelCount;            // Number of pixels in image
    GifByteType Buf[256];           // Compressed input is buffered here
    GifHashTableType *HashTable;    
} GifFilePrivateType;

bool createGIF(uint8_t *framebuffer, std::vector<uint8_t>* imageBuffer, uint16_t width, uint16_t height);
bool createBMP(uint8_t *framebuffer, std::vector<uint8_t>* imageBuffer, uint16_t width, uint16_t height);
bool createPBM(uint8_t *framebuffer, std::vector<uint8_t>* gifBuffer, uint16_t width, uint16_t height);

#endif /* _IMGLIB_H */
