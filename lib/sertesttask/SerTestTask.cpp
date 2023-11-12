#include "SerTestTask.h"
#include "GwHardware.h"
#include <WiFi.h>
#ifdef SERTEST
WiFiServer listener(10111);
const size_t BUFSIZE=10240;
void sertest(GwApi *api){
    GwLog *logger=api->getLogger();
    LOG_DEBUG(GwLog::LOG,"ser test task started");
    Serial2.begin(api->getConfig()->getInt(GwConfigDefinitions::serialBaud), SERIAL_8N1,-1,GWSERIAL_RX);
    listener.begin();
    uint8_t *buffer=new uint8_t[BUFSIZE+1];
    while (true){
        WiFiClient client = listener.available();
        if (client) {
            if (client.connected()) {
                LOG_DEBUG(GwLog::LOG,"SerTest Connected to client");
                while (true){ 
                    if (! client.connected()){
                        break;
                    }
                    int avail=client.available();
                    if (avail){
                        size_t rd=client.readBytes(buffer,avail);
                        if (rd > 0){
                            LOG_DEBUG(GwLog::DEBUG,"SerTest read %d bytes",(int)rd);
                            size_t wr=Serial2.write(buffer,rd);
                            LOG_DEBUG(GwLog::DEBUG,"SerTest written %d bytes",(int)wr);

                        }
                        else{
                            delay(50);
                        }
                    }
                    else{
                        delay(50);
                    }
                }
            }

            // close the connection:
            LOG_DEBUG(GwLog::LOG,"SerTest closing connection");
            client.stop();
        }
        delay(50);
    }
    vTaskDelete(NULL);
    return;
}

void sertestInit(GwApi *api){
    GwLog *logger=api->getLogger();
    LOG_DEBUG(GwLog::LOG,"ser test init");
    api->addUserTask(sertest,"SERTEST",4000);
}
#endif