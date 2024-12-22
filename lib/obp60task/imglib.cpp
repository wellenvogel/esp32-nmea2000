/******************************************************************************
 * Image functions:
 *   - Convert a 1bit framebuffer in RAM to
 *     - GIF, compressed, based on giflib and gif_hash
 *     - PBM, portable bitmap, very simple copy
 *     - BMP, bigger with a little bit fiddling around
 *
 * SPDX-License-Identifier: MIT
 *****************************************************************************/

#include <Arduino.h>  // needed for PROGMEM
#include <vector>
#include "GwLog.h"    // needed for logger
#include "imglib.h"

GifFilePrivateType gifprivate;

void ClearHashTable(GifHashTableType *HashTable) {
    memset(HashTable->HTable, 0xFF, HT_SIZE * sizeof(uint32_t));
}

GifHashTableType *InitHashTable(void) {
    GifHashTableType *HashTable;
    if ((HashTable = (GifHashTableType *)ps_malloc(sizeof(GifHashTableType))) == NULL) {
        return NULL;
    }
    ClearHashTable(HashTable);
    return HashTable;
}

static int KeyItem(uint32_t Item) {
    return ((Item >> 12) ^ Item) & HT_KEY_MASK;
}

void InsertHashTable(GifHashTableType *HashTable, uint32_t Key, int Code) {
    int HKey = KeyItem(Key);
    uint32_t *HTable = HashTable->HTable;
    while (HT_GET_KEY(HTable[HKey]) != 0xFFFFFL) {
        HKey = (HKey + 1) & HT_KEY_MASK;
    }
    HTable[HKey] = HT_PUT_KEY(Key) | HT_PUT_CODE(Code);
}

int ExistsHashTable(GifHashTableType *HashTable, uint32_t Key) {
    int HKey = KeyItem(Key);
    uint32_t *HTable = HashTable->HTable, HTKey;
    while ((HTKey = HT_GET_KEY(HTable[HKey])) != 0xFFFFFL) {
        if (Key == HTKey) {
            return HT_GET_CODE(HTable[HKey]);
        }
        HKey = (HKey + 1) & HT_KEY_MASK;
    }
    return -1;
}

/******************************************************************************
 Put 2 bytes (a word) into the given file in little-endian order:
******************************************************************************/
void GifPutWord(std::vector<uint8_t>* gifBuffer, uint16_t Word) {
    /*cuint8_t c[2];
    [0] = LOBYTE(Word);
    c[1] = HIBYTE(Word);
    gifBuffer->push_back(c[0]);
    gifBuffer->push_back(c[1]); */
    gifBuffer->push_back(LOBYTE(Word));
    gifBuffer->push_back(HIBYTE(Word));
}

/******************************************************************************
 This routines buffers the given characters until 255 characters are ready
 to be output. If Code is equal to -1 the buffer is flushed (EOF).
 The buffer is Dumped with first byte as its size, as GIF format requires.
******************************************************************************/
void GifBufferedOutput(std::vector<uint8_t>* gifBuffer, GifByteType *Buf, int c) {
    if (c == FLUSH_OUTPUT) {
        // Flush everything out.
        for (int i = 0; i < Buf[0] + 1; i++) {
           gifBuffer->push_back(Buf[i]);
        }
        // Mark end of compressed data, by an empty block (see GIF doc):
        Buf[0] = 0;
        gifBuffer->push_back(0);
    } else {
        if (Buf[0] == 255) {
            // Dump out this buffer - it is full:
            for (int i = 0; i < Buf[0] + 1; i++) {
                gifBuffer->push_back(Buf[i]);
            }
            Buf[0] = 0;
        }
        Buf[++Buf[0]] = c;
    }
}

/******************************************************************************
 The LZ compression output routine:
 This routine is responsible for the compression of the bit stream into
 8 bits (bytes) packets.
******************************************************************************/
void GifCompressOutput(std::vector<uint8_t>* gifBuffer, const int Code) {

    if (Code == FLUSH_OUTPUT) {
        while (gifprivate.CrntShiftState > 0) {
            // Get Rid of what is left in DWord, and flush it.
            GifBufferedOutput(gifBuffer, gifprivate.Buf, gifprivate.CrntShiftDWord & 0xff);
            gifprivate.CrntShiftDWord >>= 8;
            gifprivate.CrntShiftState -= 8;
        }
        gifprivate.CrntShiftState = 0; // For next time.
        GifBufferedOutput(gifBuffer, gifprivate.Buf, FLUSH_OUTPUT);
    } else {
        gifprivate.CrntShiftDWord |= ((long)Code) << gifprivate.CrntShiftState;
        gifprivate.CrntShiftState += gifprivate.RunningBits;
        while (gifprivate.CrntShiftState >= 8) {
            // Dump out full bytes:
            GifBufferedOutput(gifBuffer, gifprivate.Buf, gifprivate.CrntShiftDWord & 0xff);
            gifprivate.CrntShiftDWord >>= 8;
            gifprivate.CrntShiftState -= 8;
        }
    }

    /* If code cannt fit into RunningBits bits, must raise its size. Note */
    /* however that codes above LZ_MAX_CODE are used for special signaling.      */
    if (gifprivate.RunningCode >= gifprivate.MaxCode1 && Code <= LZ_MAX_CODE) {
        gifprivate.MaxCode1 = 1 << ++gifprivate.RunningBits;
    }
}

/******************************************************************************
 Setup the LZ compression for this image:
******************************************************************************/
void GifSetupCompress(std::vector<uint8_t>* gifBuffer) {
    gifBuffer->push_back(0x02);// Bits per pixel wit minimum 2

    gifprivate.Buf[0] = 0; // Nothing was output yet
    gifprivate.BitsPerPixel = 2; // Minimum is 2
    gifprivate.ClearCode = (1 << 2);
    gifprivate.EOFCode = gifprivate.ClearCode + 1;
    gifprivate.RunningCode = gifprivate.EOFCode + 1;
    gifprivate.RunningBits = 2 + 1; // Number of bits per code
    gifprivate.MaxCode1 = 1 << gifprivate.RunningBits; // Max. code + 1
    gifprivate.CrntCode = FIRST_CODE; // Signal that this is first one!
    gifprivate.CrntShiftState = 0;    // No information in CrntShiftDWord
    gifprivate.CrntShiftDWord = 0;

    GifCompressOutput(gifBuffer, gifprivate.ClearCode);
}

void createGifHeader(std::vector<uint8_t>* gifBuffer, uint16_t width, uint16_t height) {

    // SCREEN DESCRIPTOR
    gifBuffer->push_back('G');
    gifBuffer->push_back('I');
    gifBuffer->push_back('F');
    gifBuffer->push_back('8');
    gifBuffer->push_back('7');
    gifBuffer->push_back('a');

    GifPutWord(gifBuffer, width);
    GifPutWord(gifBuffer, height);

    gifBuffer->push_back(0x80 | (1 << 4));
    gifBuffer->push_back(0x00); // Index into the ColorTable for background color
    gifBuffer->push_back(0x00); // Pixel Aspect Ratio

    // Colormap
    gifBuffer->push_back(0xff); // Color 0
    gifBuffer->push_back(0xff);
    gifBuffer->push_back(0xff);
    gifBuffer->push_back(0x00); // Color 1
    gifBuffer->push_back(0x00);
    gifBuffer->push_back(0x00);

    // IMAGE DESCRIPTOR
    gifBuffer->push_back(DESCRIPTOR_INTRODUCER);

    GifPutWord(gifBuffer, 0);
    GifPutWord(gifBuffer, 0);
    GifPutWord(gifBuffer, width);
    GifPutWord(gifBuffer, height);

    gifBuffer->push_back(0x00); // No colormap here , we use the global one
}

/******************************************************************************
 The LZ compression routine:
 This version compresses the given buffer Line of length LineLen.
 This routine can be called a few times (one per scan line, for example), in
 order to complete the whole image.
******************************************************************************/
void GifCompressLine(std::vector<uint8_t>* gifBuffer, const GifPixelType *Line, const int LineLen) {
    int i = 0, CrntCode;
    GifHashTableType *HashTable;

    HashTable = gifprivate.HashTable;

    if (gifprivate.CrntCode == FIRST_CODE) { // Its first time!
        CrntCode = Line[i++];
    } else {
        CrntCode =
            gifprivate.CrntCode; // Get last code in compression
    }
    while (i < LineLen) { // Decode LineLen items
        GifPixelType Pixel = Line[i++]; // Get next pixel from stream.
        /* Form a new unique key to search hash table for the code
         * combines CrntCode as Prefix string with Pixel as postfix
         * char.
         */
        int NewCode;
        unsigned long NewKey = (((uint32_t)CrntCode) << 8) + Pixel;
        if ((NewCode = ExistsHashTable(HashTable, NewKey)) >= 0) {
            /* This Key is already there, or the string is old one,
             * so simple take new code as our CrntCode:
             */
            CrntCode = NewCode;
        } else {
            /* Put it in hash table, output the prefix code, and
             * make our CrntCode equal to Pixel.
             */
            GifCompressOutput(gifBuffer, CrntCode);
            CrntCode = Pixel;

            /* If however the HashTable if full, we send a clear
             * first and Clear the hash table.
             */
            if (gifprivate.RunningCode >= LZ_MAX_CODE) {
                // Time to do some clearance:
                GifCompressOutput(gifBuffer, gifprivate.ClearCode);
                gifprivate.RunningCode = gifprivate.EOFCode + 1;
                gifprivate.RunningBits = gifprivate.BitsPerPixel + 1;
                gifprivate.MaxCode1 = 1 << gifprivate.RunningBits;
                ClearHashTable(HashTable);
            } else {
                // Put this unique key with its relative Code in hash table:
                InsertHashTable(HashTable, NewKey, gifprivate.RunningCode++);
            }
        }
    }

    // Preserve the current state of the compression algorithm:
    gifprivate.CrntCode = CrntCode;

    if (gifprivate.PixelCount == 0) {
        // We are done - output last Code and flush output buffers:
        GifCompressOutput(gifBuffer, CrntCode);
        GifCompressOutput(gifBuffer, gifprivate.EOFCode);
        GifCompressOutput(gifBuffer, FLUSH_OUTPUT);
    }
}

bool createBMP(uint8_t *frameBuffer, std::vector<uint8_t>* imageBuffer, uint16_t width, uint16_t height) {
    // For BMP the line size has to be a multiple of 4 bytes.
    // So padding is needed. Also the lines have to be in reverded
    // order compared to plain buffer

    // BMP header for black-and-white image (1 bit per pixel)
    const uint8_t bmp_header[] PROGMEM = {
      // BITMAPFILEHEADER (14 Bytes)
      0x42, 0x4D,             // bfType: 'BM' signature
      0x2e, 0x3d, 0x00, 0x00, // bfSize: file size in bytes
      0x00, 0x00,             // bfReserved1
      0x00, 0x00,             // bfReserved2
      0x3e, 0x00, 0x00, 0x00, // bfOffBits: offset in bytes to pixeldata
      // BITMAPINFOHEADER (40 Bytes)
      0x28, 0x00, 0x00, 0x00, // biSize: DIB header size
      (uint8_t)LOBYTE(width), (uint8_t)HIBYTE(width), 0x00, 0x00,   // biWidth
      (uint8_t)LOBYTE(height), (uint8_t)HIBYTE(height), 0x00, 0x00, // biHeight
      0x01, 0x00, // biPlanes: Number of color planes (1)
      0x01, 0x00, // biBitCount: Color depth (1 bit per pixel)
      0x00, 0x00, 0x00, 0x00, // biCompression: Compression (none)
      0xf0, 0x3c, 0x00, 0x00, // biSizeImage: Image data size (calculate)
      0x13, 0x0b, 0x00, 0x00, // biXPelsPerMeter: Horizontal resolution (2835 pixels/meter)
      0x13, 0x0b, 0x00, 0x00, // biYPelsPerMeter: Vertical resolution (2835 pixels/meter)
      0x02, 0x00, 0x00, 0x00, // biClrUsed: Colors in color palette (2)
      0x00, 0x00, 0x00, 0x00, // biClrImportant: Important colors (all)
      // PALETTE: COLORTRIPLES of RGBQUAD (n * 4 Bytes)
      0x00, 0x00, 0x00, 0x00, // Color palette: Black
      0xff, 0xff, 0xff, 0x00  // Color palette: White
    };
    size_t bmp_headerSize = sizeof(bmp_header);

    size_t lineSize = (width / 8);
    size_t paddingSize = 0;
    if (lineSize % 4 != 0) {
        paddingSize = 4 - lineSize % 4;
    }
    size_t imageSize = bmp_headerSize + (lineSize + paddingSize) * height;

    imageBuffer->resize(imageSize);
    memcpy(imageBuffer->data(), bmp_header, bmp_headerSize);
    for (int y = 0; y < height; y++) {
        uint8_t* srcRow = frameBuffer + (y * lineSize);
        uint8_t* destRow = imageBuffer->data() + bmp_headerSize + ((height - 1 - y) * (lineSize + paddingSize));
        memcpy(destRow, srcRow, lineSize);
        for (int j = 0; j < paddingSize; j++) {
            destRow[lineSize + j] = 0x00;
        }
    }
    return true;
}

bool createPBM(uint8_t *frameBuffer, std::vector<uint8_t>* imageBuffer, uint16_t width, uint16_t height) {
    // creates binary PBM image inside imagebuffer 
    // returns bytesize of created image 
    const char pbm_header[] PROGMEM = "P4\n#Created by OBP60\n400 300\n";
    size_t pbm_headerSize = sizeof(pbm_header) - 1; // We don't want trailing zero
    size_t imageSize = pbm_headerSize + width / 8 * height;
    imageBuffer->resize(imageSize);
    memcpy(imageBuffer->data(), pbm_header, pbm_headerSize);
    memcpy(imageBuffer->data() + pbm_headerSize, frameBuffer, width / 8 * height);
    return true;
}

bool createGIF(uint8_t *framebuffer, std::vector<uint8_t>* gifBuffer, uint16_t width, uint16_t height) {

    size_t imageSize = 0;
    uint16_t bufOffset = 0;  // Offset into imageBuffer for next write access

    gifprivate.HashTable = InitHashTable();
    if (gifprivate.HashTable == NULL) {
        return false;
    }
    gifprivate.PixelCount = width * height;

    createGifHeader(gifBuffer, width, height);

    // Reset compress algorithm parameters.
    GifSetupCompress(gifBuffer);

    gifBuffer->reserve(4096); // to avoid lots of alloactions
    GifPixelType line[width];
    for (int y = 0; y < height; y++) {
        // convert uint8_t pixels to single pixels
        for (int x = 0; x < width; x++) {
            int byteIndex = (y * width + x) / 8;
            uint8_t bitIndex = 7 - ((y * width + x) % 8);
            line[x] = (framebuffer[byteIndex] & (uint8_t)(1 << bitIndex)) == 0;
        }
        gifprivate.PixelCount -= width;
        GifCompressLine(gifBuffer, line, width);
    }

    gifBuffer->push_back(TERMINATOR_INTRODUCER);
    free((GifHashTableType *)gifprivate.HashTable);

    return true;
}
