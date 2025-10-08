#ifndef WindFunctions_h
#define WindFunctions_h
#include <Arduino.h>

#ifndef GWWINDSENSORMODBUS_RX
#define GWWINDSENSORMODBUS_RX 16
#endif
#ifndef GWWINDSENSORMODBUS_TX
#define GWWINDSENSORMODBUS_TX 17
#endif

static const char *const dirList[19] = {"   N", " NNE", "  NE", " ENE", "   E",
										" ESE", "  SE", " SSE", "   S", " SSW",
										"  SW", " WSW", "   W", " WNW", "  NW",
										" NNW", "   N", "INIT", "FAIL"};

// Control RS485 wind direction and speed sensors with arduino.
class WindFunctions
{
public:
	// Variables

	uint16_t WindSpeed;
	uint16_t WindScale;
	uint16_t WindAngle;
	uint16_t WindDirection;

	// Functions
	WindFunctions(HardwareSerial *s, uint8_t addr, unsigned int baud = 9600, int8_t rx = GWWINDSENSORMODBUS_RX, int8_t tx = GWWINDSENSORMODBUS_TX) : serial(s), address(addr)
	{
		if (serial)
			serial->begin(baud, SERIAL_8N1, rx, tx);
	};
	bool readAll() { return readAll(address); };
	bool readAll(uint8_t address);
	const char *const getWindDirection() { return dirList[WindDirection]; };
	static WindFunctions *getInstance() { return instance; };
	static WindFunctions *createInstance(HardwareSerial *s, uint8_t addr) { return instance = new WindFunctions(s, addr); };

private:
	// Functions
	size_t readN(uint8_t *buf, size_t len);
	uint16_t CRC16_2(uint8_t *buf, int16_t len);
	void addedCRC(uint8_t *buf, int len);
	HardwareSerial *serial;
	uint8_t address;
	static WindFunctions *instance;
};

#endif
