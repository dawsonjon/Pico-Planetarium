#include "planetarium.h"
#include "frame_buffer.h"

#include "ctime"
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

//#define DISPLAY_TYPE 0 //ILI934x 320x240 TFT DIsplay
#define DISPLAY_TYPE 1 //ST7796 480x320 (Needs Pico2!)

//On Pico 2W, use wifi to get the current time
#define USE_NTP_TIME 1 //use credentials defined in ~/credentials.h
#define USE_WIFI_MANAGER 1 //use WIFI Manager to get credentials


//END OF CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

s_observer observer =
{
  .field              = 90.0f,   //float field of view in degrees 
  .alt                = 80.0f,   //float altidute in degrees
  .az                 = 10.0f,  //float azimuth in degrees
  .smallest_magnitude = 6.0f,    //float smallest magnitude star to plot
  
  .latitude           = 51.0,//float latitude - latitude in degrees
  .longitude          = 0.0,  //float longitude - longitude in degrees
};

#if DISPLAY_TYPE == 0
  #include "splash_320x240.h"
  uint8_t * splash_image = (uint8_t*)splash_320x240;
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  uint16_t image[width][height];
#else
  #include "splash_480x320.h"
  uint8_t * splash_image = (uint8_t*)splash_480x320;
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  uint16_t image[width][height];
#endif

#if USE_NTP_TIME
  #include "NTPTime.h"
  NTPTime ntpTime;
  #if USE_WIFI_MANAGER
    #include "WiFiManager.h"
  #endif
#endif


ILI934X *display;
c_frame_buffer frame_buffer((uint16_t*)image, width, height);
c_planetarium planetarium(frame_buffer, width, height);

void setup() {
  Serial.begin(115200);
  configure_display();
  Serial.println("Pico Planetarium (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  #if USE_NTP_TIME
  
    #if USE_WIFI_MANAGER
      
      String title("Pico Planetarium");
      String name("PICO_PLANETARIUM_PICO2W");
      String shortname("PICO_PLANETARIUM");
      String maker("101 Things");
      String version("0.0.1");
      
      WiFiManager wm("PICO_PLANETARIUM", "password");
      wm.setContentText(title, name, shortname, maker, version);
      wm.autoConnect();
    #else
    #include "~/credentials.h"
      Serial.print("Connecting to WiFi...");
      WiFi.begin(ssid, password);
    #endif
    ntpTime.begin();
  
  #else

    //start clock
    tm timeinfo;
    timeinfo.tm_year = 125;
    timeinfo.tm_mon = 1;
    timeinfo.tm_mday = 11;
    timeinfo.tm_hour = 18;
    timeinfo.tm_min = 16;
    timeinfo.tm_sec = 0;
    timeval tv = {.tv_sec = mktime(&timeinfo)};
    settimeofday(&tv, NULL);
  
  #endif

  //Show splash screen
  display->_writeBlock(0, 0, width-1, height-1, splash_image, width*height*2);
}


void loop()
{

  #ifdef USE_NTP_TIME
  ntpTime.setTime();
  #endif

  //get time
  time_t now;
  time(&now);
  tm *current_time = gmtime(&now); 
  observer.year  = current_time->tm_year + 1900;
  observer.month = current_time->tm_mon+1; 
  observer.day   = current_time->tm_mday;
  observer.hour  = current_time->tm_hour; 
  observer.min   = current_time->tm_min; 
  observer.sec   = current_time->tm_sec;

  Serial.println("Updating");
  uint32_t start = micros();
  planetarium.update(observer);
  uint32_t elapsed = micros()-start;
  
  Serial.println(elapsed/1000);
  display->_writeBlock(0, 0, width-1, height-1, (uint8_t*)image, width*height*2);

  
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
  display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, width, height, R0DEG, DISPLAY_TYPE);
  display->setRotation(ROTATION, INVERT_COLOURS);
  display->init();
  display->powerOn(true);
  display->clear();
}
