// A library for handling real-time clocks, dates, etc.
// 2010-02-04 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
// 2012-11-08 RAM methods - idreammicro.com
// 2012-11-14 SQW/OUT methods - idreammicro.com
// 2012-01-12 DS1388 support
// 2013-08-29 ENERGIA MSP430 support
// 2023-11-24 Return value for begin() to RTC presence check  - NoWa

#include <Wire.h>
// Energia support
#ifndef ENERGIA
// #include <avr/pgmspace.h>
#else
#define pgm_read_word(data) *data
#define pgm_read_byte(data) *data
#define PROGMEM
#endif
#include "RTClib.h"
#include <Arduino.h>


////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

static const uint8_t daysInMonth [] PROGMEM = {
  31,28,31,30,31,30,31,31,30,31,30,31
};

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (long t) {
    ss = t % 60;
    t /= 60;
    mm = t % 60;
    t /= 60;
    hh = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (yOff = 0; ; ++yOff) {
        leap = yOff % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1;
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (year >= 2000)
        year -= 2000;
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint
DateTime::DateTime (const char* date, const char* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    yOff = conv2d(date + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (date[0]) {
        case 'J': m = date[1] == 'a' ? 1 : m = date[2] == 'n' ? 6 : 7; break;
        case 'F': m = 2; break;
        case 'A': m = date[2] == 'r' ? 4 : 8; break;
        case 'M': m = date[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(date + 4);
    hh = conv2d(time);
    mm = conv2d(time + 3);
    ss = conv2d(time + 6);
}

uint8_t DateTime::dayOfWeek() const {
    uint16_t day = get() / SECONDS_PER_DAY;
    return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

long DateTime::get() const {
    uint16_t days = date2days(yOff, m, d);
    return time2long(days, hh, mm, ss);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

void RTC_DS1307::adjust(const DateTime& dt) {
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.write((byte) 0);
    Wire.write(bin2bcd(dt.second()));
    Wire.write(bin2bcd(dt.minute()));
    Wire.write(bin2bcd(dt.hour()));
    Wire.write(bin2bcd(0));
    Wire.write(bin2bcd(dt.day()));
    Wire.write(bin2bcd(dt.month()));
    Wire.write(bin2bcd(dt.year() - 2000));
    Wire.write((byte) 0);
    Wire.endTransmission();
}

DateTime RTC_DS1307::now() {
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.write((byte) 0);
    Wire.endTransmission();

    Wire.requestFrom(DS1307_ADDRESS, 7);
    uint8_t ss = bcd2bin(Wire.read());
    uint8_t mm = bcd2bin(Wire.read());
    uint8_t hh = bcd2bin(Wire.read());
    Wire.read();
    uint8_t d = bcd2bin(Wire.read());
    uint8_t m = bcd2bin(Wire.read());
    uint16_t y = bcd2bin(Wire.read()) + 2000;

    return DateTime (y, m, d, hh, mm, ss);
}

void RTC_DS1307::setSqwOutLevel(uint8_t level) {
    uint8_t value = (level == LOW) ? 0x00 : (1 << RTC_DS1307__OUT);
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.write(DS1307_CONTROL_REGISTER);
    Wire.write(value);
    Wire.endTransmission();
}

void RTC_DS1307::setSqwOutSignal(Frequencies frequency) {
    uint8_t value = (1 << RTC_DS1307__SQWE);
    switch (frequency)
    {
        case Frequency_1Hz:
            // Nothing to do.
        break;
        case Frequency_4096Hz:
            value |= (1 << RTC_DS1307__RS0);
        break;
        case Frequency_8192Hz:
            value |= (1 << RTC_DS1307__RS1);
        break;
        case Frequency_32768Hz:
        default:
            value |= (1 << RTC_DS1307__RS1) | (1 << RTC_DS1307__RS0);
        break;
    }
    Wire.beginTransmission(DS1307_ADDRESS);
  	Wire.write(DS1307_CONTROL_REGISTER);
  	Wire.write(value);
    Wire.endTransmission();
}

uint8_t RTC_DS1307::readByteInRam(uint8_t address) {
    Wire.beginTransmission(DS1307_ADDRESS);
  	Wire.write(address);
    Wire.endTransmission();

    Wire.requestFrom(DS1307_ADDRESS, 1);
    uint8_t data = Wire.read();
    Wire.endTransmission();

    return data;
}

void RTC_DS1307::readBytesInRam(uint8_t address, uint8_t length, uint8_t* p_data) {
    Wire.beginTransmission(DS1307_ADDRESS);
  	Wire.write(address);
    Wire.endTransmission();

    Wire.requestFrom(DS1307_ADDRESS, (int)length);
    for (uint8_t i = 0; i < length; i++) {
        p_data[i] = Wire.read();
    }
    Wire.endTransmission();
}

void RTC_DS1307::writeByteInRam(uint8_t address, uint8_t data) {
    Wire.beginTransmission(DS1307_ADDRESS);
  	Wire.write(address);
  	Wire.write(data);
    Wire.endTransmission();
}

void RTC_DS1307::writeBytesInRam(uint8_t address, uint8_t length, uint8_t* p_data) {
    Wire.beginTransmission(DS1307_ADDRESS);
  	Wire.write(address);
  	for (uint8_t i = 0; i < length; i++) {
  	  	Wire.write(p_data[i]);
  	}
    Wire.endTransmission();
}

uint8_t RTC_DS1307::isrunning(void) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write((byte) 0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);
  uint8_t ss = Wire.read();
  return !(ss>>7);
}

////////////////////////////////////////////////////////////////////////////////
// DS 1388 implementation

void RTC_DS1388::adjust(const DateTime& dt) {
  Wire.beginTransmission(DS1388_ADDRESS);
  Wire.write((byte) 0);
  Wire.write(bin2bcd(0)); // hundreds of seconds 0x00
  Wire.write(bin2bcd(dt.second())); // 0x01
  Wire.write(bin2bcd(dt.minute())); // 0x02
  Wire.write(bin2bcd(dt.hour())); // 0x03
  Wire.write(bin2bcd(0)); // 0x04
  Wire.write(bin2bcd(dt.day())); // 0x05
  Wire.write(bin2bcd(dt.month())); // 0x06
  Wire.write(bin2bcd(dt.year() - 2000)); // 0x07
  Wire.endTransmission();

  Wire.beginTransmission(DS1388_ADDRESS);
  Wire.write((byte) 0x0b);
  Wire.write((byte) 0x00);			//clear the 'time is invalid ' flag bit (OSF)
  Wire.endTransmission();
}

DateTime RTC_DS1388::now() {
  Wire.beginTransmission(DS1388_ADDRESS);
  Wire.write((byte) 0);
  Wire.endTransmission();

  Wire.requestFrom(DS1388_ADDRESS, 8);
  uint8_t hs = bcd2bin(Wire.read() & 0x7F);  // hundreds of seconds
  uint8_t ss = bcd2bin(Wire.read() & 0x7F);
  uint8_t mm = bcd2bin(Wire.read());
  uint8_t hh = bcd2bin(Wire.read());
  Wire.read();
  uint8_t d = bcd2bin(Wire.read());
  uint8_t m = bcd2bin(Wire.read());
  uint16_t y = bcd2bin(Wire.read()) + 2000;

  return DateTime (y, m, d, hh, mm, ss);
}

uint8_t RTC_DS1388::isrunning() {
  Wire.beginTransmission(DS1388_ADDRESS);
  Wire.write((byte)0x0b);
  Wire.endTransmission();

  Wire.requestFrom(DS1388_ADDRESS, 1);
  uint8_t ss = Wire.read();
  return !(ss>>7); //OSF flag bit
}


void RTC_DS1388::EEPROMWrite(int pos, uint8_t c) {
  uint8_t rel_pos = pos % 256;
  if(pos > 255){
    Wire.beginTransmission(DS1388_ADDRESS | DS1388_EEPROM_1);
  } else {
    Wire.beginTransmission(DS1388_ADDRESS | DS1388_EEPROM_0);
  }
  // Set address
  Wire.write((byte)rel_pos);
  // Wite data
  Wire.write((byte)c);
  Wire.endTransmission();
#if defined(__MSP430G2553__)
  delay(10); // Needed on MSP430 !!
#endif
}


uint8_t RTC_DS1388::EEPROMRead(int pos) {
  uint8_t rel_pos = pos % 256;
  if(pos > 255){
    Wire.beginTransmission(DS1388_ADDRESS | DS1388_EEPROM_1);
  } else {
    Wire.beginTransmission(DS1388_ADDRESS | DS1388_EEPROM_0);
  }
  // Set address
  Wire.write((byte)rel_pos);
  Wire.endTransmission(true); // Stay open
  // Request one byte
  if(pos > 255){
    Wire.requestFrom(DS1388_ADDRESS | DS1388_EEPROM_1, 1);
  } else {
    Wire.requestFrom(DS1388_ADDRESS | DS1388_EEPROM_0, 1);
  }
  uint8_t c = Wire.read();
#if defined(__MSP430G2553__)
  delay(10); // Needed on MSP430 !!
#endif
  return c;
}


///////////////////////////////////////////////////////////////////////////////
// RTC_PCF8563 implementation
// contributed by @mariusster, see http://forum.jeelabs.net/comment/1902

void RTC_PCF8563::adjust(const DateTime& dt) {
    Wire.beginTransmission(PCF8563_ADDRESS);
    Wire.write((byte) 0);
    Wire.write((byte) 0x0);                 // control/status1
    Wire.write((byte) 0x0);                 // control/status2
    Wire.write(bin2bcd(dt.second()));       // set seconds
    Wire.write(bin2bcd(dt.minute()));       // set minutes
    Wire.write(bin2bcd(dt.hour()));         // set hour
    Wire.write(bin2bcd(dt.day()));          // set day
    Wire.write((byte) 0x01);                // set weekday
    Wire.write(bin2bcd(dt.month()));        // set month, century to 1
    Wire.write(bin2bcd(dt.year() - 2000));  // set year to 00-99
    Wire.write((byte) 0x80);                // minute alarm value reset to 00
    Wire.write((byte) 0x80);                // hour alarm value reset to 00
    Wire.write((byte) 0x80);                // day alarm value reset to 00
    Wire.write((byte) 0x80);                // weekday alarm value reset to 00
    Wire.write((byte) 0x0);                 // set freqout 0= 32768khz, 1= 1hz
    Wire.write((byte) 0x0);                 // timer off
    Wire.endTransmission();
}

DateTime RTC_PCF8563::now() {
    Wire.beginTransmission(PCF8563_ADDRESS);
    Wire.write(PCF8563_SEC_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(PCF8563_ADDRESS, 7);
    uint8_t ss = bcd2bin(Wire.read() & 0x7F);
    uint8_t mm = bcd2bin(Wire.read() & 0x7F);
    uint8_t hh = bcd2bin(Wire.read() & 0x3F);
    uint8_t d = bcd2bin(Wire.read() & 0x3F);
    Wire.read();
    uint8_t m = bcd2bin(Wire.read()& 0x1F);
    uint16_t y = bcd2bin(Wire.read()) + 2000;

    return DateTime (y, m, d, hh, mm, ss);
}


///////////////////////////////////////////////////////////////////////////////
// RTC_BQ32000 implementation

void RTC_BQ32000::adjust(const DateTime& dt) {
    Wire.beginTransmission(BQ32000_ADDRESS);
    Wire.write((byte) 0);
    Wire.write(bin2bcd(dt.second()));
    Wire.write(bin2bcd(dt.minute()));
    Wire.write(bin2bcd(dt.hour()));
    Wire.write(bin2bcd(0));
    Wire.write(bin2bcd(dt.day()));
    Wire.write(bin2bcd(dt.month()));
    Wire.write(bin2bcd(dt.year() - 2000));
    Wire.endTransmission();
}

DateTime RTC_BQ32000::now() {
    Wire.beginTransmission(BQ32000_ADDRESS);
    Wire.write((byte) 0);
    Wire.endTransmission();

    Wire.requestFrom(DS1307_ADDRESS, 7);
    uint8_t ss = bcd2bin(Wire.read());
    uint8_t mm = bcd2bin(Wire.read());
    uint8_t hh = bcd2bin(Wire.read());
    Wire.read();
    uint8_t d = bcd2bin(Wire.read());
    uint8_t m = bcd2bin(Wire.read());
    uint16_t y = bcd2bin(Wire.read()) + 2000;

    return DateTime (y, m, d, hh, mm, ss);
}

void RTC_BQ32000::setIRQ(uint8_t state) {
    /* Set IRQ square wave output state: 0=disabled, 1=1Hz, 2=512Hz.
     */
  uint8_t reg, value;
    if (state) {
      // Setting the frequency is a bit complicated on the BQ32000:
        Wire.beginTransmission(BQ32000_ADDRESS);
	Wire.write(BQ32000_SFKEY1);
	Wire.write(BQ32000_SFKEY1_VAL);
	Wire.write(BQ32000_SFKEY2_VAL);
	Wire.write((state == 1) ? BQ32000_FTF_1HZ : BQ32000_FTF_512HZ);
	Wire.endTransmission();
    }
    value = readRegister(BQ32000_CAL_CFG1);
    value = (!state) ? value & ~(1<<BQ32000__FT) : value | (1<<BQ32000__FT);
    writeRegister(BQ32000_CAL_CFG1, value);
}

void RTC_BQ32000::setIRQLevel(uint8_t level) {
    /* Set IRQ output level when IRQ square wave output is disabled to
     * LOW or HIGH.
     */
    uint8_t value;
    // The IRQ active level bit is in the same register as the calibration
    // settings, so we preserve its current state:
    value = readRegister(BQ32000_CAL_CFG1);
    value = (!level) ? value & ~(1<<BQ32000__OUT) : value | (1<<BQ32000__OUT);
    writeRegister(BQ32000_CAL_CFG1, value);
}

void RTC_BQ32000::setCalibration(int8_t value) {
    /* Sets the calibration value to given value in the range -31 - 31, which
     * corresponds to -126ppm - +63ppm; see table 13 in th BQ32000 datasheet.
     */
    uint8_t val;
    if (value > 31) value = 31;
    if (value < -31) value = -31;
    val = (uint8_t) (value < 0) ? -value | (1<<BQ32000__CAL_S) : value;
    val |= readRegister(BQ32000_CAL_CFG1) & ~0x3f;
    writeRegister(BQ32000_CAL_CFG1, val);
}

void RTC_BQ32000::setCharger(int state) {
    /* If using a super capacitor instead of a battery for backup power, use this
     * method to set the state of the trickle charger: 0=disabled, 1=low-voltage
     * charge, 2=high-voltage charge. In low-voltage charge mode, the super cap is
     * charged through a diode with a voltage drop of about 0.5V, so it will charge
     * up to VCC-0.5V. In high-voltage charge mode the diode is bypassed and the super
     * cap will be charged up to VCC (make sure the charge voltage does not exceed your
     * super cap's voltage rating!!).
     */
    // First disable charger regardless of state (prevents it from
    // possible starting up in the high voltage mode when the low
    // voltage mode is requested):
    uint8_t value;
    writeRegister(BQ32000_TCH2, 0);
    if (state <= 0 || state > 2) return;
    value = BQ32000_CHARGE_ENABLE;
    if (state == 2) {
        // High voltage charge enable:
        value |= (1 << BQ32000__TCFE);
    }
    writeRegister(BQ32000_CFG2, value);
    // Now enable charger:
    writeRegister(BQ32000_TCH2, 1 << BQ32000__TCH2_BIT);
}


uint8_t RTC_BQ32000::readRegister(uint8_t address) {
    /* Read and return the value in the register at the given address.
     */
    Wire.beginTransmission(BQ32000_ADDRESS);
    Wire.write((byte) address);
    Wire.endTransmission();
    Wire.requestFrom(DS1307_ADDRESS, 1);
    // Get register state:
    return Wire.read();
}

void RTC_BQ32000::writeRegister(uint8_t address, uint8_t value) {
    /* Write the given value to the register at the given address.
     */
    Wire.beginTransmission(BQ32000_ADDRESS);
    Wire.write(address);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t RTC_BQ32000::isrunning() {
    return !(readRegister(0x0)>>7);
}


////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation

long RTC_Millis::offset = 0;

void RTC_Millis::adjust(const DateTime& dt) {
    offset = dt.get() - millis() / 1000;
}

DateTime RTC_Millis::now() {
    return offset + millis() / 1000;
}

////////////////////////////////////////////////////////////////////////////////
