#pragma once
#include "NMEA0183Msg.h"
class SNMEA0183Msg : public tNMEA0183Msg{
        public:
            int sourceId;
            const char *line;
            bool isAis=false;
            SNMEA0183Msg(const char *line, int sourceId){
                this->sourceId=sourceId;
                this->line=line;
                int len=strlen(line);
                if (len >6){
                    if (strncasecmp(line,"!AIVDM",6) == 0
                        ||
                        strncasecmp(line,"!AIVDO",6) == 0
                    ) isAis=true;
                }
            }
            SNMEA0183Msg(){
                line=NULL;
            }
            String getKey(){
                if (!isAis) return MessageCode();
                if (!line) return String("      ");
                char buf[6];
                strncpy(buf,line+1,5);
                buf[5]=0;
                return String(buf);
            }
            char fromHex(char v){
                if (v >= '0' && v <= '9') return v-'0';
                if (v >= 'A' && v <= 'F') return v-'A'+10;
                if (v >= 'a' && v <= 'f') return v-'a'+10;
                return 0;
            }
            bool SetMessageCor(const char *buf)
            {
                unsigned char csMsg;
                int i = 0;
                uint8_t iData = 0;
                bool result = false;

                Clear();
                _MessageTime = millis();

                if (buf[i] != '$' && buf[i] != '!')
                    return result; // Invalid message
                Prefix = buf[i];
                i++; // Pass start prefix

                //  Serial.println(buf);
                // Set sender
                for (; iData < 2 && buf[i] != 0; i++, iData++)
                {
                    CheckSum ^= buf[i];
                    Data[iData] = buf[i];
                }

                if (buf[i] == 0)
                {
                    Clear();
                    return result;
                } // Invalid message

                Data[iData] = 0;
                iData++; // null termination for sender
                // Set message code. Read until next comma
                for (; buf[i] != ',' && buf[i] != 0 && iData < MAX_NMEA0183_MSG_LEN; i++, iData++)
                {
                    CheckSum ^= buf[i];
                    Data[iData] = buf[i];
                }
                if (buf[i] != ',')
                {
                    Clear();
                    return result;
                } // No separation after message code -> invalid message

                // Set the data and calculate checksum. Read until '*'
                for (; buf[i] != '*' && buf[i] != 0 && iData < MAX_NMEA0183_MSG_LEN; i++, iData++)
                {
                    CheckSum ^= buf[i];
                    Data[iData] = buf[i];
                    if (buf[i] == ',')
                    {                                    // New field
                        Data[iData] = 0;                 // null termination for previous field
                        if (_FieldCount >= MAX_NMEA0183_MSG_FIELDS){
                            Clear();
                            return false;
                        }
                        Fields[_FieldCount] = iData + 1; // Set start of field
                        _FieldCount++;
                    }
                }

                if (buf[i] != '*')
                {
                    Clear();
                    return false;
                }                // No checksum -> invalid message
                Data[iData] = 0; // null termination for previous field
                i++;             // Pass '*';
                csMsg = fromHex(buf[i])<< 4;
                i++;
                csMsg |= fromHex(buf[i]);

                if (csMsg == CheckSum)
                {
                    result = true;
                }
                else
                {
                    Clear();
                }

                return result;
            }
};
