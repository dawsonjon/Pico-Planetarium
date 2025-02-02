#include "clines.h"
#include "planetarium.h"
#include "constellations.h"

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
  90.0f,//float field - field of view in degrees 
  45.0f, //float alt - altidute in degrees
  180.0f, //float az - azimuth in degrees
  6.0f,  //float smallest_magnitude - smallest magnitude star to plot
  0.0, //float lst - local sidereal time in degrees
  51.0,//float latitude - latitude in degrees
  0.0  //float longitude - longitude in degrees
};

ILI934X *display;

void setup() {
  configure_display();
  Serial.println("Pico Planetarium (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  datetime_t t = {
            .year  = 2025,
            .month = 02,
            .day   = 01,
            .dotw  = 6, // 0 is Sunday
            .hour  = 20,
            .min   = 37,
            .sec   = 00
  };

  // Start the RTC
  rtc_init();
  rtc_set_datetime(&t);
}


void loop()
{

  //get time
  datetime_t t;
  rtc_get_datetime(&t);
  observer.lst = local_sidereal_time(t.year, t.month, t.day, t.hour, t.min, t.sec, observer.longitude);
  
  plot();
  sleep_ms(1000);

}


void plot_star(uint16_t x, uint16_t y, int8_t mag, uint8_t mk)
{
  uint8_t r, g, b;
  star_colour(mk, r, g, b);

  if(mag <= 1)
  {
    uint16_t colour = display->colour565(r, g, b);
    display->setPixel(x, y, colour);
    display->setPixel(x+1, y, colour);
    display->setPixel(x, y+1, colour);
    display->setPixel(x, y-1, colour);
    display->setPixel(x-1, y, colour);
    display->setPixel(x+1, y-1, colour);
    display->setPixel(x-1, y+1, colour);
    display->setPixel(x+1, y+1, colour);
    display->setPixel(x-1, y-1, colour);
    display->setPixel(x+2, y, colour);
    display->setPixel(x, y+2, colour);
    display->setPixel(x, y-2, colour);
    display->setPixel(x-2, y, colour);
  }
  else if(mag <= 2)
  {
    uint16_t colour = display->colour565(r, g, b);
    display->setPixel(x, y, colour);
    display->setPixel(x+1, y, colour);
    display->setPixel(x, y+1, colour);
    display->setPixel(x, y-1, colour);
    display->setPixel(x-1, y, colour);
  }
  else if(mag <= 3)
  {
    uint16_t colour = display->colour565(r, g, b);
    display->setPixel(x, y, display->colour565(255, 255, 255));
  } 
  else if(mag <= 4)
  {
    uint16_t colour = display->colour565(r, g, b);
    display->setPixel(x, y, display->colour565(r/2, g/2, g/2));
  } 
  else
  {
    display->setPixel(x, y, display->colour565(r/4, g/4, b/4));
  } 

}

void plot()
{
  bool dirty[240];
  for(uint16_t idx=0; idx<240; ++idx) dirty[idx] = false;

  //plot constellations
  Serial.println("plotting constellations");
  uint16_t cline_colour = display->colour565(0, 0, 64);
  static uint16_t cline_x1[256];
  static uint16_t cline_y1[256];
  static uint16_t cline_x2[256];
  static uint16_t cline_y2[256];
  uint16_t num_clines_plotted = plot_constellations(cline_x1, cline_y1, cline_x2, cline_y2, dirty, 256, 320, 240, observer);

  //plot stars
  Serial.println("plotting stars");
  static uint16_t star_x[1024];
  static uint16_t star_y[1024];
  static int8_t star_mag[1024];
  static uint8_t star_col[1024];
  uint16_t num_stars_plotted = plot_stars(star_x, star_y, star_mag, star_col, dirty, 1024, 320, 240, observer);

  //plot constellation names
  static uint16_t constellation_x[num_constellations];
  static uint16_t constellation_y[num_constellations];
  static bool visible[num_constellations];
  plot_constellation_names(constellation_x, constellation_y, visible, dirty, observer, 320, 240);

  Serial.println(num_stars_plotted);

  //update display
  Serial.println("updating display");
  for(uint16_t yy=0; yy<240-8; ++yy)
  {
    if(!dirty[yy]) continue;
    display->fillRect(0, yy, 1, 320, display->colour565(5, 0, 10));
    Serial.println(yy);

    /*
    for(uint16_t idx=0; idx < num_clines_plotted; ++idx)
    {
      const uint16_t x1 = cline_x1[idx];
      const uint16_t y1 = cline_y1[idx];
      const uint16_t x2 = cline_x2[idx];
      const uint16_t y2 = cline_y2[idx];
      if(std::max(y1, y2)!=yy) continue;
      display->drawLine(x1, y1, x2, y2, cline_colour);
      display->setPixel(x1, y1, COLOUR_WHITE);
      display->setPixel(x2, y2, COLOUR_WHITE);
      for(uint16_t idx=0; idx < num_stars_plotted; ++idx)
      {
        const uint16_t x = star_x[idx];
        const uint16_t y = star_y[idx];
        const uint8_t mag = star_mag[idx];
        const uint8_t mk = star_col[idx];
        if(y==std::min(y2, y1)) plot_star(x, y, mag, mk);
      }
    }
    */

    for(uint16_t idx=0; idx < num_stars_plotted; ++idx)
    {
      const uint16_t x = star_x[idx];
      const uint16_t y = star_y[idx];
      const int8_t mag = star_mag[idx];
      const uint8_t mk = star_col[idx];
      if(y >= yy-2 && y <= yy+2) plot_star(x, y, mag, mk);
    }

    /*
    for(uint16_t idx=0; idx < num_constellations; ++idx)
    {
      if(visible[idx])
      {
        const uint16_t x = constellation_x[idx];
        const uint16_t y = constellation_y[idx];
        if(y+8==yy)
        {
          display->drawString(x, y, font_8x5, constellation_names[idx], COLOUR_WHITE, COLOUR_BLACK);
        }
      }
    }
    */
  }
  char buffer[100];
  snprintf(buffer, 100, "lat: %.1f lon: %.1f lst %.1f", observer.latitude, observer.longitude, observer.lst);
  display->fillRect(0, 239-8, 8, 320, COLOUR_BLACK);
  display->drawString(0, 239-8, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);
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
