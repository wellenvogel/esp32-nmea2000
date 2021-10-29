#include "GwBuffer.h"

void GwBuffer::lp(const char *fkt, int p)
{
    LOG_DEBUG(GwLog::DEBUG + 1, "Buffer[%s]: buf=%p,wp=%d,rp=%d,used=%d,free=%d, p=%d",
              fkt, buffer, offset(writePointer), offset(readPointer), usedSpace(), freeSpace(), p);
}

GwBuffer::GwBuffer(GwLog *logger,size_t bufferSize)
{
    this->logger = logger;
    this->bufferSize=bufferSize;
    this->buffer=new uint8_t[bufferSize];
    writePointer = buffer;
    readPointer = buffer;
}
GwBuffer::~GwBuffer(){
    delete buffer;
}
void GwBuffer::reset()
{
    writePointer = buffer;
    readPointer = buffer;
    lp("reset");
}
size_t GwBuffer::freeSpace()
{
    if (readPointer <= writePointer){
        return readPointer+bufferSize-writePointer-1;
    }
    return readPointer - writePointer - 1;
}
size_t GwBuffer::usedSpace()
{
    if (readPointer <= writePointer)
        return writePointer - readPointer;
    return writePointer+bufferSize-readPointer;
}
size_t GwBuffer::addData(const uint8_t *data, size_t len, bool addPartial)
{
    lp("addDataE", len);
    if (len == 0)
        return 0;
    if (freeSpace() < len && !addPartial)
        return 0;
    size_t written = 0;    
    for (int i=0;i<2;i++){
        size_t currentFree=freeSpace();    
        size_t toWrite=len-written;
        if (toWrite > currentFree) toWrite=currentFree;
        if (toWrite > (bufferSize - offset(writePointer))) {
            toWrite=bufferSize - offset(writePointer);
        }
        if (toWrite != 0){
            memcpy(writePointer, data, toWrite);
            written+=toWrite;
            data += toWrite;
            writePointer += toWrite;
            if (offset(writePointer) >= bufferSize){
                writePointer -= bufferSize;
            }
        }
        lp("addData1", toWrite);
    }
    lp("addData2", written);
    return written;
}
/**
         * write some data to the buffer writer
         * return an error if the buffer writer returned < 0
         */
GwBuffer::WriteStatus GwBuffer::fetchData(GwBufferWriter *writer, int maxLen,bool errorIf0 )
{
    lp("fetchDataE",maxLen);
    size_t len = usedSpace();
    if (maxLen > 0 && len > maxLen) len=maxLen;
    if (len == 0){
        lp("fetchData0",maxLen);
        writer->done();
        return OK;
    }
    size_t written = 0;
    for (int i=0;i<2;i++){
        size_t currentUsed=usedSpace();    
        size_t toWrite=len-written;
        if (toWrite > currentUsed) toWrite=currentUsed;
        if (toWrite > (bufferSize - offset(readPointer))) {
            toWrite=bufferSize - offset(readPointer);
        }
        lp("fetchData1", toWrite);
        if (toWrite > 0)
        {
            int rt = writer->write(readPointer, toWrite);
            lp("fetchData2", rt);
            if (rt < 0)
            {
                LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write returns error %d", rt);
                writer->done();
                return ERROR;
            }
            if (rt > toWrite)
            {
                LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write too many bytes(1) %d", rt);
                writer->done();
                return ERROR;
            }
            readPointer += rt;
            if (offset(readPointer) >= bufferSize)
                readPointer -= bufferSize;
            written += rt;
            if (rt == 0) break; //no need to try again
        }
    }
    writer->done();
    if (written == 0){
        return (errorIf0 ? ERROR : AGAIN);
    }
    return (written == len)?OK:AGAIN;
}

int GwBuffer::findChar(char x){
    lp("findChar",x);
    int of=0;
    uint8_t *p;
    for (p=readPointer; of < usedSpace();p++){
        if (offset(p) >= bufferSize) p -=bufferSize;
        if (*p == x) {
            lp("findChar1",of);
            return of;
        }
        of++;
    }
    lp("findChar2");
    return -1;
}

GwBuffer::WriteStatus GwBuffer::fetchMessage(GwBufferWriter *writer,char delimiter,bool emptyIfFull){
    int pos=findChar(delimiter);
    if (pos < 0) {
        if (!freeSpace() && emptyIfFull){
            LOG_DEBUG(GwLog::LOG,"line to long, reset, buffer=%p",buffer);
            reset();
            return ERROR;
        }
        return AGAIN;
    }
    return fetchData(writer,pos+1,true);
}