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
        void lp(const char *fkt,int p=0){
            LOG_DEBUG(GwLog::DEBUG + 1,"Buffer[%s]: buf=%p,wp=%d,rp=%d,used=%d,free=%d, p=%d",
                fkt,buffer,offset(writePointer),offset(readPointer),usedSpace(),freeSpace(),p
            );
        }
    public:
        GwBuffer(GwLog *logger){
            this->logger=logger;
        }
        void reset(){
            writePointer=buffer;
            readPointer=buffer;
            lp("reset");
        }
        size_t freeSpace(){
            if (readPointer < writePointer){
                size_t rt=BUFFER_SIZE-offset(writePointer)-1+offset(readPointer);
                return rt;        
            }
            if (readPointer == writePointer) return BUFFER_SIZE-1;
            return readPointer-writePointer-1;
        }
        size_t usedSpace(){
            if (readPointer == writePointer) return 0;
            if (readPointer < writePointer) return writePointer-readPointer;
            return BUFFER_SIZE-offset(readPointer)-1+offset(writePointer);
        }
        size_t addData(const uint8_t *data,size_t len){
            lp("addDataE",len);
            if (len == 0) return 0;
            if (freeSpace() < len) return 0;
            size_t written=0;
            if (writePointer >= readPointer){
                written=BUFFER_SIZE-offset(writePointer)-1;
                if (written > len) written=len;
                if (written) {
                    memcpy(writePointer,data,written);
                    len-=written;
                    data+=written;
                    writePointer+=written;
                    if (offset(writePointer) >= (BUFFER_SIZE-1)) writePointer=buffer;
                }
                lp("addData1",written);
                if (len <= 0) {
                    return written;
                }
            }
            //now we have the write pointer before the read pointer
            //and we did the length check before - so we can safely copy
            memcpy(writePointer,data,len);
            writePointer+=len;
            lp("addData2",len);
            return len+written;
        }
        /**
         * write some data to the buffer writer
         * return an error if the buffer writer returned < 0
         */
        WriteStatus fetchData(GwBufferWriter *writer, bool errorIf0 = true)
        {
            lp("fetchDataE");
            size_t len = usedSpace();
            if (len == 0)
                return OK;
            size_t written=0;    
            size_t plen=len;    
            if (writePointer < readPointer)
            {
                //we need to write from readPointer till end and then till writePointer-1
                plen = BUFFER_SIZE - offset(readPointer)-1;
                int rt = writer->write(readPointer, plen);
                lp("fetchData1",rt);
                if (rt < 0){
                    LOG_DEBUG(GwLog::DEBUG+1,"buffer: write returns error %d",rt);
                    return ERROR;
                }
                if (rt > plen){
                    LOG_DEBUG(GwLog::DEBUG+1,"buffer: write too many bytes(1) %d",rt);
                    return ERROR;
                }
                if (rt == 0){
                    LOG_DEBUG(GwLog::DEBUG+1,"buffer: write returns 0 (1)");
                    return (errorIf0 ? ERROR : AGAIN);
                }
                readPointer += rt;
                if (offset(readPointer) >= (BUFFER_SIZE-1))
                    readPointer = buffer;
                if (rt < plen)
                    return AGAIN;
                if (plen >= len)
                    return OK;
                len -=rt; 
                written+=rt;   
                //next part - readPointer should be at buffer now
            }
            plen=writePointer - readPointer;
            if (plen == 0) return OK;
            int rt = writer->write(readPointer, plen);
            lp("fetchData2",rt);
            if (rt < 0){
                LOG_DEBUG(GwLog::DEBUG+1,"buffer: write returns error %d",rt);
                return ERROR;
            }
            if (rt == 0){
                LOG_DEBUG(GwLog::DEBUG+1,"buffer: write returns 0 (1)");
                return (errorIf0 ? ERROR : AGAIN);
            }
            if (rt > plen){
                LOG_DEBUG(GwLog::DEBUG+1,"buffer: write too many bytes(2)");
                return ERROR;
            }
            readPointer += rt;
            if (offset(readPointer) >= (BUFFER_SIZE-1))
                readPointer = buffer;
            lp("fetchData3");
            written+=rt;    
            if (written < len)
                return AGAIN;
            return OK;
        }
};

#endif