    // General hardware definitions
    // CAN and RS485 bus pin definitions see obp60task.h

    // Direction pin for RS485 NMEA0183
    #define OBP_DIRECTION_PIN 18
    // I2C
    #define I2C_SPEED 10000UL       // 10kHz clock speed on I2C bus
    #define OBP_I2C_SDA 47
    #define OBP_I2C_SCL 21
    // DS1388 RTC
    #define DS1388_I2C_ADDR 0xD0    // Addr. 0xD0
    // BME280
    #define BME280_I2C_ADDR 0x76    // Addr. 0x76 (0x77)
    // BMP280
    #define BMP280_I2C_ADDR 0x76    // Addr. 0x76 (0x77)
    // BMP085 / BMP180
    #define BMP180_I2C_ADDR 0x77    // Addr. 0x77 (fix)
    // SHT21 / HUT21
    #define SHT21_I2C_ADDR 0x40     // Addr. 0x40 (fix)
    // AS5600
    #define AS5600_I2C_ADDR 0x36    // Addr. 0x36 (fix)
    // INA226
    #define SHUNT_VOLTAGE 0.075     // Shunt voltage in V by max. current (75mV)
    #define INA226_I2C_ADDR1 0x41   // Addr. 0x41 (fix A0 = 5V, A1 = GND) for battery
    #define INA226_I2C_ADDR2 0x44   // Addr. 0x44 (fix A0 = GND, A1 = 5V) for solar panels
    #define INA226_I2C_ADDR3 0x45   // Addr. 0x45 (fix A0 = 5V, A1 = 5V) for generator
    // Horter modules
    #define PCF8574_I2C_ADDR1 0x20  // First digital out module
    // SPI (E-Ink display, Extern Bus)
    #define OBP_SPI_CS 39
    #define OBP_SPI_DC 40
    #define OBP_SPI_RST 41
    #define OBP_SPI_BUSY 42
    #define OBP_SPI_CLK 38
    #define OBP_SPI_DIN 48
    #define SHOW_TIME 6000        // Show time for logo and WiFi QR code
    #define FULL_REFRESH_TIME 600 // Refresh cycle time in [s][600...3600] for full display update (very important healcy function)
    #define MAX_PAGE_NUMBER 10    // Max number of pages for show data
    #define FONT1 "Ubuntu_Bold8pt7b"
    #define FONT2 "Ubuntu_Bold24pt7b"
    #define FONT3 "Ubuntu_Bold32pt7b"
    #define FONT4 "DSEG7Classic_BoldItalic80pt7b"

    // GPS (NEO-6M, NEO-M8N)
    #define OBP_GPS_RX 2
    #define OBP_GPS_TX 1
    // 1Wire (DS18B20)
    #define OBP_1WIRE 6         // External 1Wire
    // Buzzer
    #define OBP_BUZZER 16
    #define TONE1 1500          // 1500Hz
    #define TONE2 2500          // 2500Hz
    #define TONE3 3500          // 3500Hz
    #define TONE4 4000          // 4000Hz
    // Analog Input
    #define OBP_ANALOG0 4       // Voltage power supplay
    #define MIN_VOLTAGE 9.0     // Min voltage for under voltage detection (then goto deep sleep)
    #define POWER_FAIL_TIME 2   // in [ms] Accept min voltage until 2 x 1ms (for under voltage gaps by engine start)
    // Touch buttons
    #define TOUCHTHRESHOLD 50000// Touch sensitivity, lower values more sensitiv
    #define TP1 14              // Left outside
    #define TP2 13
    #define TP3 12
    #define TP4 11
    #define TP5 10
    #define TP6 9               // Right ouside

    // Flash LED (1x WS2812B)
    #define NUM_FLASH_LED 1         // Number of flash LED
    #define OBP_FLASH_LED 7         // GPIO port
    // Backlight LEDs (6x WS2812B)
    #define NUM_BACKLIGHT_LED 6     // Numebr of Backlight LEDs
    #define OBP_BACKLIGHT_LED 15    // GPIO port
    // Power Rail
    #define OBP_POWER_50 5


