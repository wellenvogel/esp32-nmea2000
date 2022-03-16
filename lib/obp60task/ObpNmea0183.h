#pragma once
#include "NMEA0183.h"
#include "GwNmea0183Msg.h"

class ObpNmea0183 : public tNMEA0183
{
public:
    bool GetMessageCor(SNMEA0183Msg &NMEA0183Msg)
    {
        if (!IsOpen())
            return false;

        bool result = false;

        while (port->available() > 0 && !result)
        {
            int NewByte = port->read();
            if (NewByte == '$' || NewByte == '!')
            { // Message start
                MsgInStarted = true;
                MsgInPos = 0;
                MsgInBuf[MsgInPos] = NewByte;
                MsgInPos++;
            }
            else if (MsgInStarted)
            {
                MsgInBuf[MsgInPos] = NewByte;
                if (NewByte == '*')
                    MsgCheckSumStartPos = MsgInPos;
                MsgInPos++;
                if (MsgCheckSumStartPos != SIZE_MAX and MsgCheckSumStartPos + 3 == MsgInPos)
                {                           // We have full checksum and so full message
                    MsgInBuf[MsgInPos] = 0; // add null termination
                    if (NMEA0183Msg.SetMessageCor(MsgInBuf))
                    {
                        NMEA0183Msg.SourceID = SourceID;
                        result = true;
                    }
                    MsgInStarted = false;
                    MsgInPos = 0;
                    MsgCheckSumStartPos = SIZE_MAX;
                }
                if (MsgInPos >= MAX_NMEA0183_MSG_BUF_LEN)
                { // Too may chars in message. Start from beginning
                    MsgInStarted = false;
                    MsgInPos = 0;
                    MsgCheckSumStartPos = SIZE_MAX;
                }
            }
        }

        return result;
    }
};
