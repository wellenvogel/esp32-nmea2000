#ifndef _GWBUFFER_H
#define _GWBUFFER_H
#include <string.h>
#include <stdint.h>
#include "GwLog.h"

class GwBuffer;
class GwBufferWriter{
    public:
        int id=0; //can be set be users
        virtual int write(const uint8_t *buffer,size_t len)=0;
        virtual void done(){}
        virtual ~GwBufferWriter(){};
};

class GwMessageFetcher{
    public:
        int id=0;
        virtual bool handleBuffer(GwBuffer *buffer)=0;
        virtual size_t fetchMessageToBuffer(GwBuffer *gwbuffer,uint8_t *buffer, size_t bufferLen,char delimiter);  
};

/**
 * an implementation of a
 * buffer to safely inserte data if it fits
 * and to write out data if possible
 */
typedef size_t (*GwBufferHandleFunction)(uint8_t *buffer, size_t len, void *param);
class GwBuffer{
    public:
        static const size_t TX_BUFFER_SIZE=1620; // app. 20 NMEA messages
        static const size_t RX_BUFFER_SIZE=400;  // enough for 1 NMEA message or actisense message
        typedef enum {
            OK,
            ERROR,
            AGAIN
        } WriteStatus;
    private:
        size_t bufferSize;
        uint8_t *buffer;
        uint8_t *writePointer;
        uint8_t *readPointer;
        size_t offset(uint8_t* ptr){
            return (size_t)(ptr-buffer);
        }
        GwLog *logger;
        void lp(const char *fkt,int p=0);
    public:
        GwBuffer(GwLog *logger,size_t bufferSize);
        ~GwBuffer();
        void reset(String reason="");
        size_t freeSpace();
        size_t usedSpace();
        size_t addData(const uint8_t *data,size_t len,bool addPartial=false);
        size_t fillData(int maxLen,GwBufferHandleFunction handler, void *param);
        int read();
        int peek();
        size_t fetchData(int maxLen,GwBufferHandleFunction handler, void *param);
        /**
         * write some data to the buffer writer
         * return an error if the buffer writer returned < 0
         */
        WriteStatus fetchData(GwBufferWriter *writer, int maxLen=-1,bool errorIf0 = true);
        /**
         * find the first occurance of x in the buffer, -1 if not found
         */
        int findChar(char x);
        WriteStatus fetchMessage(GwBufferWriter *writer,char delimiter,bool emptyIfFull=true);
};

#endif