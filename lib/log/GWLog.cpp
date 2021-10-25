#include "GwLog.h"

class DefaultLogWriter: public GwLogWriter{
    public:
        virtual ~DefaultLogWriter(){};
        virtual void write(const char *data){
            Serial.print(data);
        }
};

GwLog::GwLog(int level, GwLogWriter *writer){
    logLevel=level;
    if (writer == NULL) writer=new DefaultLogWriter();
    this->writer=writer;
}
void GwLog::logString(const char *fmt,...){
    if (! writer) return;
    va_list args;
    va_start(args,fmt);
    vsnprintf(buffer,99,fmt,args);
    writer->write(prefix.c_str());
    writer->write(buffer);
    writer->write("\n");
}
void GwLog::logDebug(int level,const char *fmt,...){
    if (level > logLevel) return;
    if (! writer) return;
    va_list args;
    va_start(args,fmt);
    vsnprintf(buffer,99,fmt,args);
    writer->write(prefix.c_str());
    writer->write(buffer);
    writer->write("\n");
}
void GwLog::setWriter(GwLogWriter *writer){
    if (this->writer) delete this->writer;
    this->writer=writer;
}
