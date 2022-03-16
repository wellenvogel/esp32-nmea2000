    // General hardware definitions
    // CAN bus pin definitions see GwOBP60Task.h

    // Direction pin for RS485 NMEA0183
    #define OBP_DIRECTION_PIN 27
    // SeaTalk
    #define OBP_SEATALK_TX 2
    #define OBP_SEATALK_RX 15
    // I2C (MCP23017, BME280, BMP280, SHT21)
    #define OBP_I2C_SDA 21
    #define OBP_I2C_SCL 22
    // Extension Port MCP23017
    #define MCP23017_I2C_ADDR 0x20  // Addr. 0 is 0x20
    // BME280
    #define BME280_I2C_ADDR 0x76    // Addr. 0x76 (0x77)
    // BMP280
    #define BMP280_I2C_ADDR 0x77    // Addr. 0x77
    // BMP085 / BMP180
    #define BMP280_I2C_ADDR 0x77    // Addr. 0x77 (fix)
    // SHT21 / HUT21
    #define SHT21_I2C_ADDR 0x40     // Addr. 0x40 (fix)
    // SPI (E-Ink display, Extern Bus)
    #define OBP_SPI_CS 5
    #define OBP_SPI_DC 17
    #define OBP_SPI_RST 16
    #define OBP_SPI_BUSY 4
    #define OBP_SPI_CLK 18
    #define OBP_SPI_DIN 23
    #define SHOW_TIME 6000        // Show time for logo and WiFi QR code
    #define FULL_REFRESH_TIME 600 // Refresh cycle time in [s][600...3600] for full display update (very important healcy function)
    #define MAX_PAGE_NUMBER 10    // Max number of pages for show data
    #define FONT1 "Ubuntu_Bold8pt7b"
    #define FONT2 "Ubuntu_Bold24pt7b"
    #define FONT3 "Ubuntu_Bold32pt7b"
    #define FONT4 "DSEG7Classic_BoldItalic80pt7b"

    // GPS (NEO-6M)
     #define OBP_GPS_TX 35      // Read only GPS data
    // TTP229 Touch Pad Controller (!!No I2C!!)
    #define TTP_SDO 25
    #define TTP_SCL 33
    // 1Wire (DS18B20)
    #define OBP_1WIRE 32        // External 1Wire
    // Buzzer
    #define OBP_BUZZER 19
    #define TONE1 1500          // 1500Hz
    #define TONE2 2500          // 2500Hz
    #define TONE3 3500          // 3500Hz
    #define TONE4 4000          // 4000Hz
    // Analog Input
    #define OBP_ANALOG0 34      // Voltage power supplay
    #define OBP_ANALOG1 36      // Analog In 1
    #define OBP_ANALOG2 39      // Analog In 2
    #define MIN_VOLTAGE 9.0     // Min voltage for under voltage detection (then goto deep sleep)
    #define POWER_FAIL_TIME 2   // in [ms] Accept min voltage until 2 x 1ms (for under voltage gaps by engine start)
    // Extension Port PA
    #define PA0 0               // Digital Out 1
    #define PA1 1               // Digital Out 2
    #define PA2 2               // Flash LED
    #define PA3 3               // Backlight LEDs
    #define PA4 4               // Digital In 1
    #define PA5 5               // Digital In 2
    #define PA6 6               // Power Rail 5.0V
    #define PA7 7               // Power Rail 3.3V
    // Extension Port PB
    #define PB0 8               // Extension Connector
    #define PB1 9               // Extension Connector
    #define PB2 10              // Extension Connector
    #define PB3 11              // Extension Connector
    #define PB4 12              // Extension Connector
    #define PB5 13              // Extension Connector
    #define PB6 14              // Extension Connector
    #define PB7 15              // Extension Connector

    // Extension Port Digital Out
    #define OBP_DIGITAL_OUT1 PA0
    #define OBP_DIGITAL_OUT2 PA1
    // Extension Port LED
    #define OBP_FLASH_LED PA2
    // Extension Port Backlight LEDs
    #define OBP_BACKLIGHT_LED PA3
    // Extension Port Digital In
    #define OBP_DIGITAL_IN1 PA4
    #define OBP_DIGITAL_IN2 PA5
    // Extension Port Power Rails
    #define OBP_POWER_50 PA6
    #define OBP_POWER_33 PA7

