#ifndef _OBP60EXTENSIONPORT_H
#define _OBP60EXTENSIONPORT_H

#include <Arduino.h>
#include "OBP60Hardware.h"

MCP23017 mcp = MCP23017(MCP23017_I2C_ADDR);

// SPI pin definitions for E-Ink display
GxIO_Class io(SPI, OBP_SPI_CS, OBP_SPI_DC, OBP_SPI_RST);  // SPI, CS, DC, RST
GxEPD_Class display(io, OBP_SPI_RST, OBP_SPI_BUSY);       // io, RST, BUSY

// Global vars
int outA = 0;                   // Outport Byte A
int outB = 0;                   // Outport Byte B
bool blinkingLED = false;       // Enable / disable blinking flash LED
int uvDuration = 0;             // Under voltage duration in n x 100ms
uint buzPower = 50;             // Buzzer loudness in [%]

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

void buzzer(uint frequency, uint power, uint duration){
    if(frequency > 8000){   // Max 8000Hz
        frequency = 8000;
    }
    if(power > 100){        // Max 100%
        power = 100;
    }
    if(duration > 1000){    // Max 1000ms
        duration = 1000;
    }
    
    pinMode(OBP_BUZZER, OUTPUT);
    ledcSetup(0, frequency, 8);         // Ch 0, ferquency in Hz, 8 Bit resolution of PWM
    ledcAttachPin(OBP_BUZZER, 0);
    ledcWrite(0, int(power * 1.28));    // 50% duty cycle are 100%
    delay(duration);
    ledcWrite(0, 0);                    // 0% duty cycle are 0%
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
        buzzer(TONE4, buzPower, 20);                   // Buzzer tone 4kHz 20% 20ms
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

#endif