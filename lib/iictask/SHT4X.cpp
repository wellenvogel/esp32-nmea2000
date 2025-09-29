#include "GwSHTXX.h"
#ifdef _GWSHT4X

uint8_t crc8(const uint8_t *data, int len) {
    /*
     *
     * CRC-8 formula from page 14 of SHT spec pdf
     *
     * Test data 0xBE, 0xEF should yield 0x92
     *
     * Initialization data 0xFF
     * Polynomial 0x31 (x8 + x5 +x4 +1)
     * Final XOR 0x00
     */

    const uint8_t POLYNOMIAL(0x31);
    uint8_t crc(0xFF);

    for (int j = len; j; --j) {
        crc ^= *data++;

        for (int i = 8; i; --i) {
            crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
        }
    }
    return crc;
}

bool SHT4X::begin(TwoWire* wire, uint8_t addr) {
    _addr = addr;
    _wire = wire;
    int error;
    _wire->beginTransmission(addr);
    error = _wire->endTransmission();
    if (error == 0) {
        return true;
    }
    return false;
}

bool SHT4X::update() {
    uint8_t readbuffer[6];
    uint8_t cmd       = SHT4x_NOHEAT_HIGHPRECISION;
    uint16_t duration = 10;

    if (_heater == SHT4X_NO_HEATER) {
        if (_precision == SHT4X_HIGH_PRECISION) {
            cmd      = SHT4x_NOHEAT_HIGHPRECISION;
            duration = 10;
        }
        if (_precision == SHT4X_MED_PRECISION) {
            cmd      = SHT4x_NOHEAT_MEDPRECISION;
            duration = 5;
        }
        if (_precision == SHT4X_LOW_PRECISION) {
            cmd      = SHT4x_NOHEAT_LOWPRECISION;
            duration = 2;
        }
    }

    if (_heater == SHT4X_HIGH_HEATER_1S) {
        cmd      = SHT4x_HIGHHEAT_1S;
        duration = 1100;
    }
    if (_heater == SHT4X_HIGH_HEATER_100MS) {
        cmd      = SHT4x_HIGHHEAT_100MS;
        duration = 110;
    }

    if (_heater == SHT4X_MED_HEATER_1S) {
        cmd      = SHT4x_MEDHEAT_1S;
        duration = 1100;
    }
    if (_heater == SHT4X_MED_HEATER_100MS) {
        cmd      = SHT4x_MEDHEAT_100MS;
        duration = 110;
    }

    if (_heater == SHT4X_LOW_HEATER_1S) {
        cmd      = SHT4x_LOWHEAT_1S;
        duration = 1100;
    }
    if (_heater == SHT4X_LOW_HEATER_100MS) {
        cmd      = SHT4x_LOWHEAT_100MS;
        duration = 110;
    }
    // _i2c.writeByte(_addr, cmd, 1);
    _wire->beginTransmission(_addr);
    _wire->write(cmd);
    _wire->write(1);
    _wire->endTransmission();


    delay(duration);

    _wire->requestFrom(_addr, (uint8_t)6);

    for (uint16_t i = 0; i < 6; i++) {
        readbuffer[i] = _wire->read();
    }

    if (readbuffer[2] != crc8(readbuffer, 2) ||
        readbuffer[5] != crc8(readbuffer + 3, 2)) {
        return false;
    }

    float t_ticks  = (uint16_t)readbuffer[0] * 256 + (uint16_t)readbuffer[1];
    float rh_ticks = (uint16_t)readbuffer[3] * 256 + (uint16_t)readbuffer[4];

    cTemp    = -45 + 175 * t_ticks / 65535;
    humidity = -6 + 125 * rh_ticks / 65535;
    humidity = min(max(humidity, (float)0.0), (float)100.0);
    return true;
}

void SHT4X::setPrecision(sht4x_precision_t prec) {
    _precision = prec;
}

sht4x_precision_t SHT4X::getPrecision(void) {
    return _precision;
}

void SHT4X::setHeater(sht4x_heater_t heat) {
    _heater = heat;
}

sht4x_heater_t SHT4X::getHeater(void) {
    return _heater;
}
#endif