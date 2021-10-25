#ifndef _GWBUFFER_H
#define _GWBUFFER_H
#include <string.h>
#include <stdint.h>
#include "GwLog.h"

class GwBufferWriter{
    public:
        virtual int write(const uint8_t *buffer,size_t len)=0;
        virtual ~GwBufferWriter(){};
};

/**
 * an implementation of a
 * buffer to safely inserte data if it fits
 * and to write out data if possible
 */
class GwBuffer{
    public:
        static const size_t BUFFER_SIZE=1620; // app. 20 NMEA messages
        typedef enum {
            OK,
            ERROR,
            AGAIN
        } WriteStatus;
    private:
        uint8_t buffer[BUFFER_SIZE];
        uint8_t *writePointer=buffer;
        uint8_t *readPointer=buffer;
        size_t offset(uint8_t* ptr){
            return (size_t)(ptr-buffer);
        }
        GwLog *logger;
        void lp(const char *fkt,int p=0);
    public:
        GwBuffer(GwLog *logger);
        void reset();
        size_t freeSpace();
        size_t usedSpace();
        size_t addData(const uint8_t *data,size_t len);
        /**
         * write some data to the buffer writer
         * return an error if the buffer writer returned < 0
         */
        WriteStatus fetchData(GwBufferWriter *writer, bool errorIf0 = true);
};

#endif