#include "clines.h"
#include "planetarium.h"
#include "splash_320x240.h"

#include "time.h"
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
  .alt                = 10.0f,   //float altidute in degrees
  .az                 = 270.0f,  //float azimuth in degrees
  .smallest_magnitude = 6.0f,    //float smallest magnitude star to plot
  
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

  //start clock
  tm timeinfo;
  timeinfo.tm_year = 2025;
  timeinfo.tm_mon = 2;
  timeinfo.tm_mday = 11;
  timeinfo.tm_hour = 18;
  timeinfo.tm_min = 16;
  timeinfo.tm_sec = 0;
  timeval tv = {.tv_sec = mktime(&timeinfo)};
  settimeofday(&tv, NULL);

  //Show splash screen
  display->_writeBlock(0, 0, width-1, height-1, (uint8_t*)splash_320x240, width*height*2);
  sleep_ms(2000);
}


void loop()
{
  static uint16_t hour = 0;

  //get time
  time_t now;
  time(&now);
  tm *current_time = localtime(&now); 
  observer.year  = current_time->tm_year;
  observer.month = current_time->tm_mon; 
  observer.day   = current_time->tm_mday;
  observer.hour  = current_time->tm_hour; 
  observer.min   = current_time->tm_min; 
  observer.sec   = current_time->tm_sec;

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
