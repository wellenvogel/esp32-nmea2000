#include "GwLedSpiTask.h"
#include "GwHardware.h"
#include "GwApi.h"
#include <driver/spi_master.h>
#include "ColorTo3Byte.h"

/*
controlling some WS2812 using SPI
https://controllerstech.com/ws2812-leds-using-spi/

*/

typedef enum {
    LED_OFF,
    LED_GREEN,
    LED_BLUE,
    LED_RED,
    LED_WHITE
} GwLedMode;

typedef struct{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

static Color COLOR_GREEN={
    .r=0,
    .g=255,
    .b=0
};
static Color COLOR_RED={
    .r=255,
    .g=0,
    .b=0
};
static Color COLOR_BLUE={
    .r=0,
    .g=0,
    .b=255
};
static Color COLOR_WHITE={
    .r=255,
    .g=255,
    .b=255
};
static Color COLOR_BLACK={
    .r=0,
    .g=0,
    .b=0
};

static uint8_t mulcolor(uint8_t f1, uint8_t f2){
    uint16_t rt=f1;
    rt*=(uint16_t)f2;
    return rt >> 8;
}

static Color setBrightness(const Color &color,uint8_t brightness){
    //very simple for now
    Color rt=color;
    rt.g=mulcolor(rt.g,brightness);
    rt.b=mulcolor(rt.b,brightness);
    rt.r=mulcolor(rt.r,brightness);
    return rt;
}

static Color colorFromMode(GwLedMode cmode){
    switch(cmode){
        case LED_BLUE:
            return COLOR_BLUE;    
        case LED_GREEN:
            return COLOR_GREEN;
        case LED_RED:
            return COLOR_RED;
        case LED_WHITE:
            return COLOR_WHITE;        
        default:
            return COLOR_BLACK;    
    }
}



static void colorCompTo3Byte(uint8_t comp,uint8_t *buffer){
    for (int i=0;i<3;i++){
        *(buffer+i)=colorTo3Byte[comp][i];
    }
}

//depending on LED strip - handle color order
static size_t ledsToBuffer(int numLeds,const Color *leds,uint8_t *buffer){
    uint8_t *p=buffer;
    for (int i=0;i<numLeds;i++){
        colorCompTo3Byte(leds[i].r,p);
        p+=3;
        colorCompTo3Byte(leds[i].g,p);
        p+=3;
        colorCompTo3Byte(leds[i].b,p);
        p+=3;
    }
    return p-buffer;
}

#define EXIT_TASK delay(50);vTaskDelete(NULL);return;
void handleSpiLeds(GwApi *api){
    GwLog *logger=api->getLogger();
    spi_host_device_t bus;
    bool spiValid=false;
    #ifndef GWLED_SPI
        LOG_DEBUG(GwLog::LOG,"no SPI LEDs defined");
        EXIT_TASK;
    #else
    #ifndef GWLED_PIN
        #error "GWLED_PIN not defined"
    #endif
    #if GWLED_SPI == 1
        #ifdef GWSPI1_CLK
        #error "GWLED_SPI is 1 but SPI bus one is used by GWSPI1_CLK"
        #endif
        bus=SPI2_HOST;
        spiValid=true;
    #endif
    #if GWLED_SPI == 2
        #ifdef GWSPI2_CLK
        #error "GWLED_SPI is 2 but SPI bus two is used by GWSPI2_CLK"
        #endif
        bus=SPI3_HOST;
        spiValid=true;
    #endif
    if (! spiValid){
        LOG_DEBUG(GwLog::ERROR,"invalid SPI bus defined for SPI leds %s",GWSTRINGIFY(GWLED_SPI))
        EXIT_TASK;
    }
    LOG_DEBUG(GwLog::ERROR,"SpiLed task started");
    uint8_t ledPin=GWLED_PIN;
    spi_bus_config_t buscfg = {
        .mosi_io_num = ledPin,
        .miso_io_num = -1,
        .sclk_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
        };   
    esp_err_t err=spi_bus_initialize(bus,&buscfg,SPI_DMA_CH_AUTO);    
    if (err != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"unable to initialize SPI bus %d,mosi=%d, error=%d",
        (int)bus,ledPin,(int)err);
        EXIT_TASK;
    }
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans =0,
        .clock_speed_hz = 2500000, //2.5 Mhz 
        .input_delay_ns =0,
        .spics_io_num = -1,             //CS pin
        .queue_size = 1 //see https://github.com/espressif/esp-idf/issues/9450
        };
    spi_device_handle_t device;
    err=spi_bus_add_device(bus,&devcfg,&device);
    if (err != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"unable to add device to SPI bus %d,mosi=%d, error=%d",
        (int)bus,ledPin,(int)err);
        EXIT_TASK;
    }
    const int NUMLEDS=2;
    Color leds[NUMLEDS];
    size_t bufferSize=NUMLEDS*3*3;
    uint8_t *outbuffer=(uint8_t*) heap_caps_malloc(bufferSize,MALLOC_CAP_DMA);
    if (! outbuffer){
        LOG_DEBUG(GwLog::ERROR,"unable to allocate %d bytes of DMA buffer",(int)bufferSize);
        EXIT_TASK;
    }
    uint8_t brightness=api->getConfig()->getInt(GwConfigDefinitions::ledBrightness,128);
    GwLedMode currentMode=LED_GREEN;
    bool first=true;
    int apiResult=0;
    int count=0;
    int seccount=0;
    int modeIndex=0;
    GwLedMode modes[4]={LED_WHITE,LED_GREEN,LED_RED,LED_BLUE};
    while (true)
    {
        delay(50);
        count++;
        if (count > 20){ //1s repeat
            first=true;
            count=0;
            seccount++;
        }
        GwLedMode newMode = currentMode;
        /*
        IButtonTask buttonState = api->taskInterfaces()->get<IButtonTask>(apiResult);
        if (apiResult >= 0 )
        {
            switch (buttonState.state)
            {
            case IButtonTask::PRESSED_5:
                newMode = LED_BLUE;
                break;
            case IButtonTask::PRESSED_10:
                newMode = LED_RED;
                break;
            default:
                newMode = LED_GREEN;
                break;
            }
        }
        else
        {
            newMode = LED_WHITE;
        }
        */
        if (seccount > 5){
            newMode=modes[modeIndex];
            modeIndex++;
            if (modeIndex >= 4) modeIndex=0;
            seccount=0;
            first=true;
        }
        if (newMode != currentMode || first)
        {
            first=false;
            leds[0] = setBrightness(colorFromMode(newMode),brightness);
            leds[1] = leds[0];
            LOG_DEBUG(GwLog::DEBUG,"(%d) mode=%d,setting color g=%d,r=%d,b=%d",modeIndex,(int)newMode,leds[0].g,leds[0].r,leds[0].b);
            ledsToBuffer(NUMLEDS,leds,outbuffer);
            struct spi_transaction_t ta = {
                .flags = 0,
                .cmd = 0,
                .addr = 0,
                .length = bufferSize*8,
                .rxlength = 0,
                .tx_buffer = outbuffer
                };
            int64_t now=esp_timer_get_time();
            esp_err_t ret = spi_device_transmit(device, &ta);
            int64_t end=esp_timer_get_time();
            if (ret != ESP_OK){
                LOG_DEBUG(GwLog::ERROR,"unable to send led data: %d",(int)ret);
            }
            else{
                LOG_DEBUG(GwLog::DEBUG,"successfully send led data for %d leds, %lld us",NUMLEDS,end-now);
            }
            currentMode = newMode;
        }
    }
    vTaskDelete(NULL);
    #endif
}