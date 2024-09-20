// A library for handling real-time clocks, dates, etc.
// 2010-02-04 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
// 2012-11-08 RAM methods - idreammicro.com
// 2012-11-14 SQW/OUT methods - idreammicro.com
// 2023-11-24 Return value for begin() to RTC presence check  - NoWa

// DS1307
#define DS1307_ADDRESS          0x68
#define DS1307_CONTROL_REGISTER 0x07
#define DS1307_RAM_REGISTER     0x08

// DS1307 Control register bits.
#define RTC_DS1307__RS0         0x00
#define RTC_DS1307__RS1         0x01
#define RTC_DS1307__SQWE        0x04
#define RTC_DS1307__OUT         0x07

// DS1388
#define DS1388_ADDRESS          0x68

// DS1388 Control register bits
#define DS1388_EEPROM_0         0x01
#define DS1388_EEPROM_1         0x02

// PCF8563
#define PCF8563_ADDRESS         0x51
#define PCF8563_SEC_ADDR        0x02

// BQ32000
#define BQ32000_ADDRESS         0x68
// BQ32000 register addresses:
#define BQ32000_CAL_CFG1        0x07
#define BQ32000_TCH2            0x08
#define BQ32000_CFG2            0x09
#define BQ32000_SFKEY1          0x20
#define BQ32000_SFKEY2          0x21
#define BQ32000_SFR             0x22
// BQ32000 config bits:
#define BQ32000__OUT            0x07 // CAL_CFG1 - IRQ active state
#define BQ32000__FT             0x06 // CAL_CFG1 - IRQ square wave enable
#define BQ32000__CAL_S          0x05 // CAL_CFG1 - Calibration sign
#define BQ32000__TCH2_BIT       0x05 // TCH2 - Trickle charger switch 2
#define BQ32000__TCFE           0x06 // CFG2 - Trickle FET control
// BQ32000 config values:
#define BQ32000_CHARGE_ENABLE   0x05 // CFG2 - Trickle charger switch 1 enable
#define BQ32000_SFKEY1_VAL      0x5E
#define BQ32000_SFKEY2_VAL      0xC7
#define BQ32000_FTF_1HZ         0x01
#define BQ32000_FTF_512HZ       0x00

#define SECONDS_PER_DAY         86400L

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime (long t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const char* date, const char* time);

    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long get() const;

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};

// RTC based on the DS1307 chip connected via I2C and the Wire library
class RTC_DS1307 {
public:

    // SQW/OUT frequencies.
    enum Frequencies
    {
        Frequency_1Hz,
        Frequency_4096Hz,
        Frequency_8192Hz,
        Frequency_32768Hz
    };

    static bool begin() {
        Wire.beginTransmission(DS1307_ADDRESS);
        return (Wire.endTransmission() == 0);
    }
    static void adjust(const DateTime& dt);
    static DateTime now();
    static uint8_t isrunning();

    // SQW/OUT functions.
    void setSqwOutLevel(uint8_t level);
    void setSqwOutSignal(Frequencies frequency);

    // RAM registers read/write functions. Address locations 08h to 3Fh.
    // Max length = 56 bytes.
    static uint8_t readByteInRam(uint8_t address);
    static void readBytesInRam(uint8_t address, uint8_t length, uint8_t* p_data);
    static void writeByteInRam(uint8_t address, uint8_t data);
    static void writeBytesInRam(uint8_t address, uint8_t length, uint8_t* p_data);

    // utility functions
    static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
    static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }
};


// DS1388 version
class RTC_DS1388 {
public:
    static bool begin() {
        Wire.beginTransmission(DS1388_ADDRESS);
        return (Wire.endTransmission() == 0);
    };
    static void adjust(const DateTime& dt);
    static DateTime now();
    static uint8_t isrunning();

    // EEPROM
    static void EEPROMWrite(int pos, uint8_t c);
    static uint8_t EEPROMRead(int pos);

    // utility functions
    static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
    static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }
};



// RTC based on the PCF8563 chip connected via I2C and the Wire library
// contributed by @mariusster, see http://forum.jeelabs.net/comment/1902
class RTC_PCF8563 {
public:
    static bool begin() {
        Wire.beginTransmission(PCF8563_ADDRESS);
        return (Wire.endTransmission() == 0);
    }
    static void adjust(const DateTime& dt);
    static DateTime now();

    // utility functions
    static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
    static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }
};


// TI BQ32000 I2C RTC
class RTC_BQ32000 {
public:
    static bool begin() {
        Wire.beginTransmission(BQ32000_ADDRESS);
        return (Wire.endTransmission() == 0);
    }
    static void adjust(const DateTime& dt);
    static DateTime now();
    static uint8_t isrunning();

    static void setIRQ(uint8_t state);
    /* Set IRQ output state: 0=disabled, 1=1Hz, 2=512Hz.
     */
    static void setIRQLevel(uint8_t level);
    /* Set IRQ output active state to LOW or HIGH.
     */
    static void setCalibration(int8_t value);
    /* Sets the calibration value to given value in the range -31 - 31, which
     * corresponds to -126ppm - +63ppm; see table 13 in th BQ32000 datasheet.
     */
    static void setCharger(int state);
    /* If using a super capacitor instead of a battery for backup power, use this
       method to set the state of the trickle charger: 0=disabled, 1=low-voltage
       charge, 2=high-voltage charge. In low-voltage charge mode, the super cap is
       charged through a diode with a voltage drop of about 0.5V, so it will charge
       up to VCC-0.5V. In high-voltage charge mode the diode is bypassed and the super
       cap will be charged up to VCC (make sure the charge voltage does not exceed your
       super cap's voltage rating!!). */

    // utility functions:
    static uint8_t readRegister(uint8_t address);
    static uint8_t writeRegister(uint8_t address, uint8_t value);
    static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
    static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }
};


// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this clock won't be correct once the millis() timer rolls over (>49d?)
class RTC_Millis {
public:
    static void begin(const DateTime& dt) { adjust(dt); }
    static void adjust(const DateTime& dt);
    static DateTime now();

protected:
    static long offset;
};
