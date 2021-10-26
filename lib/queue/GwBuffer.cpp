#include "GwBuffer.h"

void GwBuffer::lp(const char *fkt, int p)
{
    LOG_DEBUG(GwLog::DEBUG + 1, "Buffer[%s]: buf=%p,wp=%d,rp=%d,used=%d,free=%d, p=%d",
              fkt, buffer, offset(writePointer), offset(readPointer), usedSpace(), freeSpace(), p);
}

GwBuffer::GwBuffer(GwLog *logger,size_t bufferSize, bool rotate)
{
    this->logger = logger;
    this->rotate=rotate;
    this->bufferSize=bufferSize;
    this->buffer=new uint8_t[bufferSize];
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
    if (! rotate){
        return bufferSize-offset(writePointer)-1;
    }
    if (readPointer < writePointer)
    {
        size_t rt = bufferSize - offset(writePointer) - 1 + offset(readPointer);
        return rt;
    }
    if (readPointer == writePointer)
        return bufferSize - 1;
    return readPointer - writePointer - 1;
}
size_t GwBuffer::usedSpace()
{
    if (readPointer == writePointer)
        return 0;
    if (readPointer < writePointer)
        return writePointer - readPointer;
    return bufferSize - offset(readPointer) - 1 + offset(writePointer);
}
size_t GwBuffer::addData(const uint8_t *data, size_t len)
{
    lp("addDataE", len);
    if (len == 0)
        return 0;
    if (freeSpace() < len && rotate)
        //in rotating mode (send buffer)
        //we only fill in a message if it fit's completely
        return 0;
    size_t written = 0;
    bool canRotate=rotate && offset(readPointer) > 0;
    if (writePointer >= readPointer)
    {
        written = bufferSize - offset(writePointer) - 1;
        if (written > 0 && ! canRotate){
            //if we cannot rotate we are not allowed to write to the last byte
            //as otherwise we would not be able to distinguish between full and empty
            written--;
        }
        if (written > len)
            written = len;
        if (written)
        {
            memcpy(writePointer, data, written);
            len -= written;
            data += written;
            writePointer += written;
            if (offset(writePointer) >= (bufferSize - 1) && canRotate)
                writePointer = buffer;
        }
        lp("addData1", written);
        if (len <= 0)
        {
            return written;
        }
    }
    if (! canRotate) return written;
    //now we have the write pointer before the read pointer
    int maxLen=readPointer-writePointer-1;
    if (len < maxLen) maxLen=len;
    memcpy(writePointer, data, maxLen);
    writePointer += maxLen;
    lp("addData2", maxLen);
    return maxLen + written;
}
/**
         * write some data to the buffer writer
         * return an error if the buffer writer returned < 0
         */
GwBuffer::WriteStatus GwBuffer::fetchData(GwBufferWriter *writer, int maxLen,bool errorIf0 )
{
    lp("fetchDataE");
    size_t len = usedSpace();
    if (maxLen > 0 && len > maxLen) len=maxLen;
    if (len == 0)
        return OK;
    size_t written = 0;
    size_t plen = len;
    if (writePointer < readPointer)
    {
        //we need to write from readPointer till end and then till writePointer-1
        plen = bufferSize - offset(readPointer) - 1;
        int rt = writer->write(readPointer, plen);
        lp("fetchData1", rt);
        if (rt < 0)
        {
            LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write returns error %d", rt);
            return ERROR;
        }
        if (rt > plen)
        {
            LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write too many bytes(1) %d", rt);
            return ERROR;
        }
        if (rt == 0)
        {
            LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write returns 0 (1)");
            return (errorIf0 ? ERROR : AGAIN);
        }
        readPointer += rt;
        if (offset(readPointer) >= (bufferSize - 1))
            readPointer = buffer;
        if (rt < plen)
            return AGAIN;
        if (plen >= len)
            return OK;
        len -= rt;
        written += rt;
        //next part - readPointer should be at buffer now
    }
    plen = writePointer - readPointer;
    if (plen == 0)
        return OK;
    int rt = writer->write(readPointer, plen);
    lp("fetchData2", rt);
    if (rt < 0)
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write returns error %d", rt);
        return ERROR;
    }
    if (rt == 0)
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write returns 0 (1)");
        return (errorIf0 ? ERROR : AGAIN);
    }
    if (rt > plen)
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "buffer: write too many bytes(2)");
        return ERROR;
    }
    readPointer += rt;
    if (offset(readPointer) >= (bufferSize - 1))
        readPointer = buffer;
    lp("fetchData3");
    written += rt;
    if (written < len)
        return AGAIN;
    return OK;
}

int GwBuffer::findChar(char x){
    int offset=0;
    uint8_t *p;
    for (p=readPointer; p != writePointer && p < (buffer+bufferSize);p++){
        if (*p == x) return offset;
        offset++;
    }
    if (p >= (buffer+bufferSize)){
        //we reached the end of the buffer without "hitting" the write pointer
        //so we can start from the beginning if rotating...
        if (! rotate) return -1;
        for (p=buffer;p < writePointer && p < (buffer+bufferSize);p++){
            if (*p == x) return offset;
            offset++;
        }
    }
    return -1;
}

GwBuffer::WriteStatus GwBuffer::fetchMessage(GwBufferWriter *writer,char delimiter,bool emptyIfFull){
    int pos=findChar(delimiter);
    if (pos < 0) {
        if (!freeSpace() && emptyIfFull){
            LOG_DEBUG(GwLog::LOG,"line to long, reset");
            reset();
            return ERROR;
        }
        return AGAIN;
    }
    return fetchData(writer,pos+1,true);
}