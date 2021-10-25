#include "GwBuffer.h"

void GwBuffer::lp(const char *fkt, int p)
{
    LOG_DEBUG(GwLog::DEBUG + 1, "Buffer[%s]: buf=%p,wp=%d,rp=%d,used=%d,free=%d, p=%d",
              fkt, buffer, offset(writePointer), offset(readPointer), usedSpace(), freeSpace(), p);
}

GwBuffer::GwBuffer(GwLog *logger)
{
    this->logger = logger;
}
void GwBuffer::reset()
{
    writePointer = buffer;
    readPointer = buffer;
    lp("reset");
}
size_t GwBuffer::freeSpace()
{
    if (readPointer < writePointer)
    {
        size_t rt = BUFFER_SIZE - offset(writePointer) - 1 + offset(readPointer);
        return rt;
    }
    if (readPointer == writePointer)
        return BUFFER_SIZE - 1;
    return readPointer - writePointer - 1;
}
size_t GwBuffer::usedSpace()
{
    if (readPointer == writePointer)
        return 0;
    if (readPointer < writePointer)
        return writePointer - readPointer;
    return BUFFER_SIZE - offset(readPointer) - 1 + offset(writePointer);
}
size_t GwBuffer::addData(const uint8_t *data, size_t len)
{
    lp("addDataE", len);
    if (len == 0)
        return 0;
    if (freeSpace() < len)
        return 0;
    size_t written = 0;
    if (writePointer >= readPointer)
    {
        written = BUFFER_SIZE - offset(writePointer) - 1;
        if (written > len)
            written = len;
        if (written)
        {
            memcpy(writePointer, data, written);
            len -= written;
            data += written;
            writePointer += written;
            if (offset(writePointer) >= (BUFFER_SIZE - 1))
                writePointer = buffer;
        }
        lp("addData1", written);
        if (len <= 0)
        {
            return written;
        }
    }
    //now we have the write pointer before the read pointer
    //and we did the length check before - so we can safely copy
    memcpy(writePointer, data, len);
    writePointer += len;
    lp("addData2", len);
    return len + written;
}
/**
         * write some data to the buffer writer
         * return an error if the buffer writer returned < 0
         */
GwBuffer::WriteStatus GwBuffer::fetchData(GwBufferWriter *writer, bool errorIf0 )
{
    lp("fetchDataE");
    size_t len = usedSpace();
    if (len == 0)
        return OK;
    size_t written = 0;
    size_t plen = len;
    if (writePointer < readPointer)
    {
        //we need to write from readPointer till end and then till writePointer-1
        plen = BUFFER_SIZE - offset(readPointer) - 1;
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
        if (offset(readPointer) >= (BUFFER_SIZE - 1))
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
    if (offset(readPointer) >= (BUFFER_SIZE - 1))
        readPointer = buffer;
    lp("fetchData3");
    written += rt;
    if (written < len)
        return AGAIN;
    return OK;
}