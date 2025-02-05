#include "clines.h"
#include "planetarium.h"

#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "hardware/spi.h"
#include "ili934x.h"
#include "font_8x5.h"
#include "font_16x12.h"

//CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

#define PIN_MISO 12 //not used by TFT but part of SPI bus
#define PIN_CS   13
#define PIN_SCK  14
#define PIN_MOSI 15 
#define PIN_DC   11
#define SPI_PORT spi1

#define ROTATION R0DEG
//#define ROTATION R90DEG
//#define ROTATION R180DEG
//#define ROTATION R270DEG
//#define ROTATION MIRRORED0DEG
//#define ROTATION MIRRORED90DEG
//#define ROTATION MIRRORED180DEG
//#define ROTATION MIRRORED270DEG

#define INVERT_COLOURS false
//#define INVERT_COLOURS true

s_observer observer =
{
  .field              = 90.0f,   //float field of view in degrees 
  .alt                = 45.0f,   //float altidute in degrees
  .az                 = 180.0f,  //float azimuth in degrees
  .smallest_magnitude = 8.0f,    //float smallest magnitude star to plot
  
  .latitude           = 51.0,//float latitude - latitude in degrees
  .longitude          = 0.0,  //float longitude - longitude in degrees
};

ILI934X *display;
c_planetarium planetarium;

void setup() {
  Serial.begin(115200);
  configure_display();
  Serial.println("Pico Planetarium (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  datetime_t t = {
            .year  = 2025,
            .month = 2,
            .day   = 3,
            .dotw  = 6, // 0 is Sunday
            .hour  = 19,
            .min   = 52,
            .sec   = 00
  };

  // Start the RTC
  rtc_init();
  rtc_set_datetime(&t);
}


void loop()
{
  static uint16_t hour = 0;

  //get time
  datetime_t t;
  rtc_get_datetime(&t);
  observer.year  = t.year;
  observer.month = t.month; 
  observer.day   = t.day;
  observer.hour  = t.hour; 
  observer.min   = t.min; 
  observer.sec   = t.sec;

  //observer.hour  = hour++; 
  //observer.min   = 0; 
  //observer.sec   = 0;

  Serial.println("Updating");
  
  uint32_t start = micros();
  planetarium.update(observer);
  uint32_t elapsed = micros()-start;
  
  Serial.println(elapsed/1000);
  display->_writeBlock(0, 0, width-1, height-1, (uint8_t*)buffer, width*height*2);

  
  //sleep_ms(1000);

}

void configure_display()
{
  spi_init(SPI_PORT, 62500000);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_init(PIN_DC);
  gpio_set_dir(PIN_DC, GPIO_OUT);
  display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, 320, 240, R0DEG);
  display->setRotation(ROTATION, INVERT_COLOURS);
  display->init();
  display->powerOn(true);
  display->clear();
}
