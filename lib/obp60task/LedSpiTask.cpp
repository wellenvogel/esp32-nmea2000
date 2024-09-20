#include <FreeRTOS.h>
#include "LedSpiTask.h"
#include "GwHardware.h"
#include "GwApi.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <soc/spi_periph.h>
#include "ColorTo3Byte.h"

/*
controlling some WS2812 using SPI
https://controllerstech.com/ws2812-leds-using-spi/

*/


static uint8_t mulcolor(uint8_t f1, uint8_t f2){
    uint16_t rt=f1;
    rt*=(uint16_t)f2;
    return rt >> 8;
}

Color setBrightness(const Color &color,uint8_t brightness){
    uint16_t br255=brightness*255;
    br255=br255/100;
    //very simple for now
    Color rt=color;
    rt.g=mulcolor(rt.g,br255);
    rt.b=mulcolor(rt.b,br255);
    rt.r=mulcolor(rt.r,br255);
    return rt;
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
        colorCompTo3Byte(leds[i].g,p);
        p+=3;
        colorCompTo3Byte(leds[i].r,p);
        p+=3;
        colorCompTo3Byte(leds[i].b,p);
        p+=3;
    }
    return p-buffer;
}

/**
 * prepare a GPIO pin to be used as the data line for an led stripe
*/
bool prepareGpio(GwLog *logger, uint8_t pin){
    esp_err_t err=gpio_set_direction((gpio_num_t)pin,GPIO_MODE_OUTPUT);
    if (err != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"unable to set gpio mode for %d: %d",pin,(int)err);
        return false;
    }
    err=gpio_set_level((gpio_num_t)pin,0);
    if (err != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"unable to set gpio level for %d: %d",pin,(int)err);
        return false;
    }
    return true;
}
/**
 * initialize the SPI bus and add a device for the LED output
 * it still does not attach any PINs to the bus
 * this will be done later when sending out
 * this way we can use one hardware SPI for multiple led stripes
 * @param bus : the SPI bus
 * @param device: <out> the device handle being filled
 * @return false on error
*/
bool prepareSpi(GwLog *logger,spi_host_device_t bus,spi_device_handle_t *device){
    spi_bus_config_t buscfg = {
        .mosi_io_num = -1,
        .miso_io_num = -1,
        .sclk_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags=SPICOMMON_BUSFLAG_GPIO_PINS
        };   
    esp_err_t err=spi_bus_initialize(bus,&buscfg,SPI_DMA_CH_AUTO);    
    if (err != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"unable to initialize SPI bus %d,mosi=%d, error=%d",
        (int)bus,-1,(int)err);
        return false;
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
    err=spi_bus_add_device(bus,&devcfg,device);
    if (err != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"unable to add device to SPI bus %d,mosi=%d, error=%d",
        (int)bus,-1,(int)err);
        return false;
    }
    //slightly speed up the transactions
    //as we are the only ones using the bus we can safely acquire it forever
    err=spi_device_acquire_bus(*device,portMAX_DELAY);
    if (err != ESP_OK){
        LOG_DEBUG(GwLog::ERROR,"unable to acquire SPI bus %d,mosi=%d, error=%d",
        (int)bus,-1,(int)err);
        return false;
    }
    return true;
}

/**
 * send out a set of Color values to a connected led stripe
 * this method will block until sen dis complete
 * But as the transfer is using DMA the CPU is not busy during the wait time
 * @param pin: the IO pin to be used. Will be attached to the SPI device before and deattached after 
 * @param numLeds: the number of Color values
 * @param leds: pointer to the first Color value
 * @param bus: the SPI bus
 * @param device: the SPI device handle
 **/
bool sendToLeds(GwLog *logger, uint8_t pin, int numLeds, Color *leds, spi_host_device_t bus, spi_device_handle_t &device, uint8_t *buffer = NULL)
{
    //need to send a long reset before
    //as on S3 MOSI is high on idle on older frameworks
    //see https://github.com/espressif/esp-idf/issues/13974
    const int zeroprefix=80; //3.2us per byte 
    bool ownsBuffer = false;
    size_t bufferSize = numLeds * 3 * 3+zeroprefix;
    if (buffer == NULL)
    {
        ownsBuffer = true;
        buffer = (uint8_t *)heap_caps_malloc(bufferSize, MALLOC_CAP_DMA|MALLOC_CAP_32BIT);
        if (!buffer)
        {
            LOG_DEBUG(GwLog::ERROR, "unable to allocate %d bytes of DMA buffer", (int)bufferSize);
            return false;
        }
    }
    bool rv = true;
    for (int i=0;i<zeroprefix;i++)buffer[i]=0;
    ledsToBuffer(numLeds, leds, buffer+zeroprefix);
    struct spi_transaction_t ta = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = bufferSize * 8,
        .rxlength = 0,
        .tx_buffer = buffer};
    int64_t now = esp_timer_get_time();
    esp_rom_gpio_connect_out_signal(pin, spi_periph_signal[bus].spid_out, false, false);
    esp_err_t ret = spi_device_transmit(device, &ta);
    esp_rom_gpio_connect_out_signal(pin, SIG_GPIO_OUT_IDX, false, false);
    int64_t end = esp_timer_get_time();
    if (ret != ESP_OK)
    {
        LOG_DEBUG(GwLog::ERROR, "unable to send led data: %d", (int)ret);
        rv = false;
    }
    else
    {
        LOG_DEBUG(GwLog::DEBUG, "successfully send led data for %d leds, %lld us", numLeds, end - now);
    }
    if (ownsBuffer)
    {
        heap_caps_free(buffer);
    }
    return rv;
}

#define EXIT_TASK delay(50);vTaskDelete(NULL);return;
void handleSpiLeds(void *param){
    LedTaskData *taskData=(LedTaskData*)param;
    GwLog *logger=taskData->api->getLogger();
    LOG_DEBUG(GwLog::ERROR,"spi led task initialized");
    spi_host_device_t bus=SPI3_HOST;
    bool spiValid=false;
    LOG_DEBUG(GwLog::ERROR,"SpiLed task started");
    
    if (! prepareGpio(logger,OBP_FLASH_LED)){
        EXIT_TASK;
    }
    if (! prepareGpio(logger,OBP_BACKLIGHT_LED)){
        EXIT_TASK;
    }
    spi_device_handle_t device;
    if (! prepareSpi(logger,bus,&device)){
        EXIT_TASK;
    }
    bool first=true;
    LedInterface current;
    while (true)
    {
        LedInterface newLeds=taskData->getLedData();
        if (first || current.backlightChanged(newLeds) || current.flasChanged(newLeds)){
            first=false;
            LOG_DEBUG(GwLog::ERROR,"handle SPI leds");
            if (current.backlightChanged(newLeds) || first){
                LOG_DEBUG(GwLog::ERROR,"setting backlight r=%02d,g=%02d,b=%02d",
                 newLeds.backlight[0].r,newLeds.backlight[0].g,newLeds.backlight[0].b);
                sendToLeds(logger,OBP_BACKLIGHT_LED,newLeds.backlightLen(),newLeds.backlight,bus,device);
            }
            if (current.flasChanged(newLeds) || first){
                LOG_DEBUG(GwLog::ERROR,"setting flashr=%02d,g=%02d,b=%02d",
                 newLeds.flash[0].r,newLeds.flash[0].g,newLeds.flash[0].b);
                sendToLeds(logger,OBP_FLASH_LED,newLeds.flashLen(),newLeds.flash,bus,device);
            }
            current=newLeds;
        }
        delay(50);
    }
    vTaskDelete(NULL);
}

void createSpiLedTask(LedTaskData *param){
    xTaskCreate(handleSpiLeds,"handleLeds",4000,param,3,NULL);
}