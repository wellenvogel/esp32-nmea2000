#include "GwBuffer.h"

void GwBuffer::lp(const char *fkt, int p)
{
    LOG_DEBUG(GwLog::DEBUG+2 , "Buffer[%s:%s]: buf=%p,wp=%d,rp=%d,used=%d,free=%d, p=%d",
              name.c_str(),fkt, buffer, offset(writePointer), offset(readPointer), usedSpace(), freeSpace(), p);
}

GwBuffer::GwBuffer(GwLog *logger,size_t bufferSize,String name)
{
    LOG_DEBUG(GwLog::DEBUG,"creating new buffer %p=%s of size %d",this,name.c_str(),(int)bufferSize);
    this->logger = logger;
    this->bufferSize=bufferSize;
    this->buffer=new uint8_t[bufferSize];
    writePointer = buffer;
    readPointer = buffer;
    this->name=name;
}
GwBuffer::~GwBuffer(){
    delete buffer;
}
void GwBuffer::reset(String reason)
{
    LOG_DEBUG(GwLog::LOG,"reseting buffer %s, reason %s",this->name.c_str(),reason.c_str());
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
int GwBuffer::read(){
    if (! usedSpace()) return -1;
    int rt=*readPointer;
    readPointer++;
    if (offset(readPointer) >= bufferSize)
                readPointer -= bufferSize;
    lp("read",rt);
    return rt;            
}
int GwBuffer::peek(){
    if (! usedSpace()) return -1;
    return *readPointer;
}
size_t GwBuffer::addData(const uint8_t *data, size_t len, bool addPartial)
{
    lp("addDataE", len);
    if (len == 0)
        return 0;
    if (freeSpace() < len && !addPartial){
        lp("addDataR0",0);
        return 0;
    }
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
    lp("addDataR", written);
    return written;
}

size_t GwBuffer::fetchData(int maxLen, GwBufferHandleFunction handler, void *param){
    lp("fetchE",maxLen);
    if (usedSpace() < 1) {
        lp("fetchR0",0);
        return 0;
    }
    size_t len=0;
    if (writePointer > readPointer){
        len=writePointer-readPointer;
    }
    else{
        len=bufferSize-offset(readPointer);
    }
    if (maxLen >= 0 && maxLen < len) len=maxLen;
    size_t handled=handler(readPointer,len,param);
    if (handled > len) handled=len;
    readPointer+=handled;
    if (offset(readPointer) >= bufferSize ) readPointer-=bufferSize;
    lp("fetchR",handled);
    return handled;
}
size_t GwBuffer::fillData(int maxLen, GwBufferHandleFunction handler, void *param)
{
    lp("fillDataE",maxLen);
    if (freeSpace() < 1)
        return 0;
    size_t len = 0;
    if (writePointer >= readPointer)
    {
        len = bufferSize - offset(writePointer);
        if (len > 0 && offset(readPointer) == 0) len--;
    }
    else
    {
        len = readPointer - writePointer - 1;
    }
    if (maxLen >= 0 && maxLen < len)
        len = maxLen;
    if (len == 0) return 0;    
    size_t handled = handler(writePointer, len,param);
    if (handled > len)
        handled = len;
    writePointer += handled;
    if (offset(writePointer) >= bufferSize)
        writePointer -= bufferSize;
    lp("fillDataR",handled);    
    return handled;
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
    lp("findChar2",-1);
    return -1;
}


size_t GwMessageFetcher::fetchMessageToBuffer(GwBuffer *gwbuffer,uint8_t *buffer, size_t bufferLen,char delimiter){
    int offset=gwbuffer->findChar(delimiter);
    if (offset <0) {
        if (! gwbuffer->freeSpace()){
            gwbuffer->reset(F("Message to long for input buffer"));
        }
        return 0;
    }
    offset+=1; //we include the delimiter
    if (offset >= bufferLen){
        gwbuffer->reset(F("Message to long for message buffer"));
        return 0;
    }
    size_t fetched=0;
    while (fetched < offset){
        size_t max=offset-fetched;
        size_t rd=gwbuffer->fetchData(max,[](uint8_t *buffer, size_t len, void *p)->size_t{
            memcpy(p,buffer,len);
            return len;
        },buffer+fetched);
        if (rd == 0){
            //some internal error
            gwbuffer->reset(F("Internal fetch error"));
            return 0;
        }
        fetched+=rd;
    }
    buffer[fetched]=0;
    return fetched;
}