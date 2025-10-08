#include "GwWindSensorModbusTask.h"
#include "WindFunctions.h"
#include "NMEA0183Msg.h"
#include "N2kMessages.h"

void initWindSensorModbusTask(GwApi *api)
{
  GwLog *logger = api->getLogger();
  GwConfigHandler *config = api->getConfig();
  // Initialize COM port with the correct baud rate, parity, etc.
  // TODO: Make sure to use the configured COM port, baud rate, address, RX and TX pins, etc.
  WindFunctions::createInstance(&Serial1, 1);
  LOG_DEBUG(GwLog::LOG, "windsensormodbustask initialized");
  api->addUserTask(runWindSensorModbusTask, "windsensormodbustask", 4000);
  return;
}

void runWindSensorModbusTask(GwApi *api)
{
  GwLog *logger = api->getLogger();
  LOG_DEBUG(GwLog::LOG, "windsensor task started");
  WindFunctions *wind = WindFunctions::getInstance();
  while (true)
  {
    LOG_DEBUG(GwLog::LOG, "Reading wind sensor");
    if (wind->readAll())
    {
      LOG_DEBUG(GwLog::LOG, "Wind speed: %d", wind->WindSpeed);
      LOG_DEBUG(GwLog::LOG, "Wind scale: %d", wind->WindScale);
      LOG_DEBUG(GwLog::LOG, "Wind angle: %d", wind->WindAngle);
      LOG_DEBUG(GwLog::LOG, "Wind direction: %s", wind->getWindDirection());

      // TODO: Figure out how to do this with NMEA2000 - I could not find the right XDR type
      tNMEA0183Msg msg;
      msg.Init("MWV", "WI");
      msg.AddDoubleField(wind->WindAngle / 10.0);
      msg.AddStrField("R");
      msg.AddDoubleField(wind->WindSpeed / 10.0);
      msg.AddStrField("N");
      msg.AddStrField("A");
      api->sendNMEA0183Message(msg);
      /*       tN2kMsg n2kMsg;
            SetN2kWindSpeed(n2kMsg, 15, wind.WindSpeed / 10.0, wind.WindAngle / 10.0, tN2kWindReference::N2kWind_Apparent);
            api->sendN2kMessage(n2kMsg);
       */
    }
    vTaskDelay(1000);
  }
  vTaskDelete(NULL);
  return;
}