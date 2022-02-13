#include <Arduino.h>
#include "OBP60Hardware.h"
#include "OBP60ExtensionPort.h"

#include "Ubuntu_Bold8pt7b.h"
#include "Ubuntu_Bold20pt7b.h"
#include "Ubuntu_Bold32pt7b.h"
#include "DSEG7Classic-BoldItalic16pt7b.h"
#include "DSEG7Classic-BoldItalic42pt7b.h"
#include "DSEG7Classic-BoldItalic60pt7b.h"

MCP23017 mcp = MCP23017(MCP23017_I2C_ADDR);

// SPI pin definitions for E-Ink display
GxIO_Class io(SPI, OBP_SPI_CS, OBP_SPI_DC, OBP_SPI_RST);  // SPI, CS, DC, RST
GxEPD_Class display(io, OBP_SPI_RST, OBP_SPI_BUSY);       // io, RST, BUSY

// Global vars
int outA = 0;                   // Outport Byte A
int outB = 0;                   // Outport Byte B
bool blinkingLED = false;       // Enable / disable blinking flash LED
int uvDuration = 0;             // Under voltage duration in n x 100ms

void MCP23017Init()
{
    mcp.init();
    mcp.portMode(MCP23017Port::A, 0b00110000); // Port A, 0 = out, 1 = in
    mcp.portMode(MCP23017Port::B, 0b11110000); // Port B, 0 = out, 1 = in

    // Extension Port A set defaults
    setPortPin(OBP_DIGITAL_OUT1, false);  // PA0
    setPortPin(OBP_DIGITAL_OUT2, false);  // PA1
    setPortPin(OBP_FLASH_LED, false);     // PA2
    setPortPin(OBP_BACKLIGHT_LED, false); // PA3
    setPortPin(OBP_POWER_50, true);       // PA6
    setPortPin(OBP_POWER_33, true);       // PA7

    // Extension Port B set defaults
    setPortPin(PB0, false); // PB0
    setPortPin(PB1, false); // PB1
    setPortPin(PB2, false); // PB2
    setPortPin(PB3, false); // PB3
}

void setPortPin(uint pin, bool value){

    if(pin <=7){
    
        outA &= ~(1 << pin);     // Clear bit
    
        outA |= (value << pin);  // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_A, outA);
    }
    else{
        pin = pin - 8;
        outB &= ~(1 << pin);     // Clear bit
        outB |= (value << pin);  // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_B, outB);
    }
}

void togglePortPin(uint pin){
    if(pin <=7){
        outA ^= (1 << pin);     // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_A, outA);
    }
    else{
        pin = pin - 8;
        outB ^= (1 << pin);     // Set bit
        mcp.writeRegister(MCP23017Register::GPIO_B, outB);
    }
}

void blinkingFlashLED(){
    noInterrupts();
    if(blinkingLED == true){
        togglePortPin(OBP_FLASH_LED);
    }
    else{
        setPortPin(OBP_FLASH_LED, false);
    }     
    interrupts();
}

void setBlinkingLED(bool on){
    blinkingLED = on;
}

uint buzzerpower = 50;

void buzzer(uint frequency, uint duration){
    if(frequency > 8000){   // Max 8000Hz
        frequency = 8000;
    }
    if(buzzerpower > 100){        // Max 100%
        buzzerpower = 100;
    }
    if(duration > 1000){    // Max 1000ms
        duration = 1000;
    }
    
    pinMode(OBP_BUZZER, OUTPUT);
    ledcSetup(0, frequency, 8);         // Ch 0, ferquency in Hz, 8 Bit resolution of PWM
    ledcAttachPin(OBP_BUZZER, 0);
    ledcWrite(0, uint(buzzerpower * 1.28));    // 50% duty cycle are 100%
    delay(duration);
    ledcWrite(0, 0);                    // 0% duty cycle are 0%
}

void setBuzzerPower(uint power){
    buzzerpower = power;
}

void underVoltageDetection(){
    noInterrupts();
    float actVoltage = (float(analogRead(OBP_ANALOG0)) * 3.3 / 4096 + 0.17) * 20;   // Vin = 1/20 
    if(actVoltage < MIN_VOLTAGE){
        uvDuration ++;
    }
    else{
        uvDuration = 0;
    }
    if(uvDuration > POWER_FAIL_TIME){
        setPortPin(OBP_BACKLIGHT_LED, false);   // Backlight Off
        setPortPin(OBP_FLASH_LED, false);       // Flash LED Off
        buzzer(TONE4, 20);                      // Buzzer tone 4kHz 20% 20ms
        setPortPin(OBP_POWER_50, false);        // Power rail 5.0V Off
        setPortPin(OBP_POWER_33, false);        // Power rail 3.3V Off
        // Shutdown EInk display
        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false); // Partial update
 //       display._sleep();                       // Display shut dow
        // Stop system
        while(true){
            esp_deep_sleep_start();             // Deep Sleep without weakup. Weakup only after power cycle (restart).
        }
    }    
    interrupts();
}
